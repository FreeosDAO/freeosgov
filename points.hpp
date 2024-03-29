//#pragma once
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include "freeosgov.hpp"
#include "tables.hpp"

#include <cmath>

using namespace eosio;
using namespace freedao;
using namespace std;

/** @defgroup points Points
 *  These Actions and functions are related to managing the POINTs ledger each participant.
 *  @{
 */

/**
 * Action creates a new token
 * 
 * @param issuer The account that will issue the token.
 * @param maximum_supply The maximum amount of tokens that can be created.
 */
void freeosgov::create(const name &issuer, const asset &maximum_supply) {
  require_auth(get_self());

  auto sym = maximum_supply.symbol;
  check(sym.is_valid(), "invalid symbol name");
  check(maximum_supply.is_valid(), "invalid supply");
  check(maximum_supply.amount > 0, "max-supply must be positive");

  stats statstable(get_self(), sym.code().raw());
  auto existing = statstable.find(sym.code().raw());
  check(existing == statstable.end(), "token with symbol already exists");

  statstable.emplace(get_self(), [&](auto &s) {
    s.supply.symbol = maximum_supply.symbol;
    s.max_supply = maximum_supply;
    s.issuer = issuer;
  });
}


/**
 * Function checks that the issuer is the account that's calling the function, that the quantity is valid,
 * that the quantity is positive, that the quantity is less than the maximum supply, and then it adds
 * the quantity to the supply and adds the quantity to the issuer's balance.
 * 
 * This function is called by the 'mint' action which applies a whitelist of accounts that are permitted to issue POINTs.
 * 
 * @param to The account to which the tokens are issued.
 * @param quantity The amount of tokens to issue.
 * @param memo A string that can be used to store additional information about the transaction.
 */
void freeosgov::issue(const name &to, const asset &quantity, const string &memo) {
  auto sym = quantity.symbol;
  check(sym.is_valid(), "invalid symbol name");
  check(memo.size() <= 256, "memo has more than 256 bytes");

  stats statstable(get_self(), sym.code().raw());
  auto existing = statstable.find(sym.code().raw());
  check(existing != statstable.end(),
        "token with symbol does not exist, create token before issue");
  const auto &st = *existing;
  check(to == st.issuer, "tokens can only be issued to issuer account");

  check(quantity.is_valid(), "invalid quantity");
  check(quantity.amount > 0, "must issue positive quantity");

  check(quantity.symbol == st.supply.symbol, "symbol precision mismatch");
  check(quantity.amount <= st.max_supply.amount - st.supply.amount,
        "quantity exceeds available supply");

  statstable.modify(st, same_payer, [&](auto &s) { s.supply += quantity; });

  add_balance(st.issuer, quantity, st.issuer);
}


/**
 * Function takes an asset and a memo, checks that the asset is valid, and then subtracts the asset from the
 * supply and the issuer's balance
 * 
 * This is a standard token management function that is not exposed as an action. It is called by the 'burn' action which
 * applies a whitelist of accounts that are permitted to reduce the supply of a token.
 * 
 * @param quantity The amount of POINTs to retire.
 * @param memo A string that can be used to store additional information about the transaction.
 */
void freeosgov::retire(const asset &quantity, const string &memo) {
  auto sym = quantity.symbol;
  check(sym.is_valid(), "invalid symbol name");
  check(memo.size() <= 256, "memo has more than 256 bytes");

  stats statstable(get_self(), sym.code().raw());
  auto existing = statstable.find(sym.code().raw());
  check(existing != statstable.end(), "token with symbol does not exist");
  const auto &st = *existing;

  check(quantity.is_valid(), "invalid quantity");
  check(quantity.amount > 0, "must retire positive quantity of POINTs");

  check(quantity.symbol == st.supply.symbol, "symbol precision mismatch");

  statstable.modify(st, same_payer, [&](auto &s) { s.supply -= quantity; });

  sub_balance(st.issuer, quantity);
}


/**
 * Action checks if the `from` account is in the `transferers` table, and if it is, it calls the `transfer`
 * function. It is therefore a wrapper for the transfer function that applies a whitelist of which accounts are
 * permitted to transfer POINTs.
 * 
 * @param from The account that is sending the tokens.
 * @param to The account that will receive the tokens.
 * @param quantity The amount of tokens to be allocated.
 * @param memo The memo is a string that can be used to store additional information about the
 * transfer.
 */
void freeosgov::allocate(const name &from, const name &to, const asset &quantity, const string &memo) {
  require_auth(from);

  // check if the 'from' account is in the transferer whitelist
  transferers_index transferers_table(get_self(), get_self().value);
  auto transferer_iterator = transferers_table.find(from.value);

  check(transferer_iterator != transferers_table.end(),
        "the allocate action is protected by transferers whitelist");

  // if the 'from' user is in the transferers table then call the transfer function
  transfer(from, to, quantity, memo);
}


/**
 * Action is called by the user to unlock some of their locked POINTs.
 * It checks that the user has not already unlocked in the current iteration, and if not, it
 * calculates the amount to be unlocked based on the unlock_percentage set in the system table, and then it
 * unlocks the amount.
 * 
 * @param user the account name of the user who is unlocking
 * 
 * @return Nothing.
 */
void freeosgov::unlock(const name &user) {
  require_auth(user);

  // user-activity-driven background process. Note that tick() checks the masterswitch
  tick();

  // get the current iteration
  uint32_t this_iteration = current_iteration();
  check(this_iteration > 0, "unlocking is not possible at this time, please try later");

  // calculate the amount to be unvested - get the percentage for the iteration
  system_index system_table(get_self(), get_self().value);
  auto system_iterator = system_table.begin();
  check(system_iterator != system_table.end(), "system record not found");
  uint32_t unlock_percent = system_iterator->unlockpercent;

  // check that the unvest percentage is within limits
  check(unlock_percent > 0 && unlock_percent <= 100,
        "locked POINTs cannot be unlocked in this claim period. Please try during next claim period");

  // has the user unlocked in this iteration? - consult the unvest history table
  unvest_index unvest_table(get_self(), user.value);
  auto unvest_iterator = unvest_table.begin();
  // if the unvest record exists for the current iteration then the user has already unvested,
  // so is not eligible to unvest again
  if (unvest_iterator != unvest_table.end()) {
    check(unvest_iterator->iteration_number != this_iteration,
        "user has already unlocked in this iteration");
  }

  // do the unlocking
  // get the user's unvested POINT balance
  asset locked_balance = asset(0, POINT_CURRENCY_SYMBOL);
  lockaccounts locked_accounts_table(get_self(), user.value);
  auto locked_account_iterator = locked_accounts_table.begin();

  if (locked_account_iterator != locked_accounts_table.end()) {
    locked_balance = locked_account_iterator->balance;
  }

  // if user's locked balance is 0 then nothing to do, so return
  if (locked_balance.amount == 0) {
    return;
  }

  // calculate the amount of locked POINTs to convert to liquid POINTs
  // Warning: these calculations use mixed-type arithmetic. Any changes need to
  // be thoroughly tested.

  double percentage = unlock_percent / 100.0;
  double locked_amount = locked_balance.amount / ((double) POINT_UNIT_MULTIPLIER);
  double percentage_applied = locked_amount * percentage;
  double adjusted_amount = ceil(percentage_applied); // rounding up to whole units
  uint64_t adjusted_units = adjusted_amount * POINT_UNIT_MULTIPLIER;

  // to prevent rounding up to more than the locked point balance, apply this adjustment
  // this will bring the locked balance to zero
  if (adjusted_units > locked_balance.amount) {
    adjusted_units = locked_balance.amount;
  }

  asset converted_points = asset(adjusted_units, POINT_CURRENCY_SYMBOL);

  std::string memo = std::string("unlocking POINTs by ");
  memo.append(user.to_string());

  // Issue the required amount to the freeos account
  if (converted_points.amount > 0) {
    issue(get_self(), converted_points, memo);

    // transfer liquid POINTs to user
    transfer(get_self(), user, converted_points, memo);

  // subtract the amount transferred from the unvested record
  locked_accounts_table.modify(locked_account_iterator, get_self(), [&](auto &v) { v.balance -= converted_points; });
  }
  
  // record the unlock event in the unvest history table
  unvest_iterator = unvest_table.begin();
  if (unvest_iterator == unvest_table.end()) {
    unvest_table.emplace(get_self(), [&](auto &unvest) {
      unvest.iteration_number = this_iteration;
    });
  } else {
    unvest_table.modify(unvest_iterator, same_payer, [&](auto &unvest) {
      unvest.iteration_number = this_iteration;
    });
  }
}


/**
 * Action checks if the `minter` account is in the `minters` table, and if it is, it calls the `issue`
 * function
 * 
 * @param minter the account that is allowed to mint tokens
 * @param to The account to receive the tokens.
 * @param quantity The amount of tokens to be minted.
 * @param memo a string that can be used to store additional information about the transaction.
 */
void freeosgov::mint(const name &minter, const name &to, const asset &quantity, const string &memo) {
  // check if the 'to' account is in the minter whitelist
  minters_index minters_table(get_self(), get_self().value);
  auto minter_iterator = minters_table.find(minter.value);

  check(minter_iterator != minters_table.end(), "the mint action is protected by minters whitelist");

  require_auth(minter);

  // if the 'to' user is in the minters table then call the issue function
  issue(to, quantity, memo);
}


/**
 * Action checks if the account that is calling the burn action is in the burners table, and if it is, it
 * calls the retire function
 * 
 * @param burner The account that is burning the tokens.
 * @param quantity The amount of tokens to burn.
 * @param memo The memo is a string that is passed along with the transfer.
 */
void freeosgov::burn(const name &burner, const asset &quantity, const string &memo) {
  // check if the 'burner' account is in the burner whitelist
  burners_index burners_table(get_self(), get_self().value);
  auto burner_iterator = burners_table.find(burner.value);

  check(burner_iterator != burners_table.end(), "the burn action is protected by burners whitelist");

  require_auth(burner);

  // if the 'to' user is in the burners table then call the retire function
  retire(quantity, memo);
}


/**
 * Function to transfer POINT tokens from sender to receiver.
 * This is a wrapper to the transfer function that applies a whitelist of which accounts are permitted to transfer POINTs.
 * 
 * @param from The account that is sending the tokens.
 * @param to The account to which the tokens are transferred.
 * @param quantity The amount of tokens to transfer.
 * @param memo A string that can be used to store additional information about the transfer.
 */
void freeosgov::transfer(const name &from, const name &to, const asset &quantity, const string &memo) {
  check(from != to, "cannot transfer to self");
  // require_auth(from);
  check(is_account(to), "to account does not exist");

  auto sym = quantity.symbol.code();
  stats statstable(get_self(), sym.raw());
  const auto &st = statstable.get(sym.raw());

  require_recipient(from);
  require_recipient(to);

  check(quantity.is_valid(), "invalid quantity");
  check(quantity.amount > 0, "must transfer positive quantity");
  check(quantity.symbol == st.supply.symbol, "symbol precision mismatch");
  check(memo.size() <= 256, "memo has more than 256 bytes");

  auto payer = has_auth(to) ? to : from;

  sub_balance(from, quantity);
  add_balance(to, quantity, payer);
}


/**
 * It subtracts the amount of the asset from the balance of the account
 * 
 * @param owner The account that will be debited
 * @param value The amount of tokens to be transferred.
 */
void freeosgov::sub_balance(const name &owner, const asset &value) {
  accounts from_acnts(get_self(), owner.value);

  const auto &from =
      from_acnts.get(value.symbol.code().raw(), "no balance object found");
  check(from.balance.amount >= value.amount, "overdrawn balance");

  from_acnts.modify(from, owner, [&](auto &a) { a.balance -= value; });
}


/**
 * It adds the value of the asset to the balance of the owner
 * 
 * @param owner The account to which the tokens are being transferred.
 * @param value The amount of tokens to be transferred.
 * @param ram_payer The account that will pay for the RAM used by the new account.
 */
void freeosgov::add_balance(const name &owner, const asset &value,
                         const name &ram_payer) {
  accounts to_acnts(get_self(), owner.value);
  auto to = to_acnts.find(value.symbol.code().raw());
  if (to == to_acnts.end()) {
    to_acnts.emplace(ram_payer, [&](auto &a) { a.balance = value; });
  } else {
    to_acnts.modify(to, same_payer, [&](auto &a) { a.balance += value; });
  }
}


/**
 * Action takes a quantity of POINT tokens from the user and issues an equivalent amount of FREEBI tokens
 * to the user
 * 
 * @param owner the account that is minting the FREEBI tokens
 * @param quantity the amount of POINTs to be converted to FREEBIs
 */
void freeosgov::mintfreebi(const name &owner, const asset &quantity) {
  require_auth(owner);

  // check that system is operational (masterswitch parameter set to "1")
  check(check_master_switch(), MSG_FREEOS_SYSTEM_NOT_AVAILABLE);

  uint32_t this_iteration = current_iteration();
    
  // is the system operational?
  check(this_iteration != 0, "The freeos system is not available at this time");

  // is the 'owner' user verified?
  check(is_user_verified(owner), "minting is restricted to verified users");

  auto sym = quantity.symbol;
  check(sym == POINT_CURRENCY_SYMBOL, "invalid symbol name");

  // check that the user has the appropriate balance to mint from
  // get POINT balance
  asset points_balance = asset(0, POINT_CURRENCY_SYMBOL);  // default value of 0 POINTs
  accounts points_accounts_table(get_self(), owner.value);
  auto points_iterator = points_accounts_table.find(POINT_CURRENCY_SYMBOL.code().raw());
  if (points_iterator != points_accounts_table.end()) {
    points_balance = points_iterator->balance;
  }
  check(points_balance.amount >= quantity.amount, "user has insufficient POINTs balance");

  stats statstable(get_self(), sym.code().raw());
  auto existing = statstable.find(sym.code().raw());
  check(existing != statstable.end(), "token with symbol does not exist");
  const auto &st = *existing;

  check(quantity.is_valid(), "invalid quantity");
  check(quantity.amount > 0, "must convert positive quantity");

  // decrease the supply
  check(st.supply.amount >= quantity.amount, "mintfreebi: there are insufficient POINTs to burn");
  statstable.modify(st, same_payer, [&](auto &s) {
    s.supply -= quantity;
  });

  // decrease owner's balance of non-exchangeable tokens
  sub_balance(owner, quantity);

  // Issue exchangeable tokens
  asset exchangeable_amount = asset(quantity.amount, FREEBI_CURRENCY_SYMBOL);
  std::string memo = std::string("minting");

  // ask FREEBI contract to issue an equivalent amount of FREEBI tokens to the freeosgov account
  string freebi_tokens_contract = get_parameter(name("freebitokens"));
  action issue_action = action(
      permission_level{get_self(), "active"_n}, name(freebi_tokens_contract),
      "issue"_n, std::make_tuple(get_self(), exchangeable_amount, memo));

  issue_action.send();

  // transfer FREEBI tokens to the owner
  action transfer_action = action(
      permission_level{get_self(), "active"_n}, name(freebi_tokens_contract),
      "transfer"_n,
      std::make_tuple(get_self(), owner, exchangeable_amount, memo));

  transfer_action.send();
}


/**
 * The function calculates the mint fee in the currency that the user is paying with.
 * 
 * @param mint_quantity the amount of tokens to be exchanged for FREEOS
 * @param mint_fee_currency symbol: the currency the user is paying the mint fee with
 */
asset freeosgov::calculate_mint_fee(asset &mint_quantity, symbol mint_fee_currency) {

  asset mintfee;
  double mint_fee_percent;

  double mintfee_amount = 0;    // amount of currency, e.g. 123.4567 represents 123.4567 FREEOS
  int64_t mintfee_units = 0;    // units of currency, e.g. 1234567 represents 123.4567 FREEOS

  check(mint_quantity.amount > 0, "invalid mint quantity");
  
  // retrieve the latest mint fee percent from the last reward record
  rewards_index rewards_table(get_self(), get_self().value);
  auto reward_iterator = rewards_table.rbegin();
  check(reward_iterator != rewards_table.rend(), "latest reward record not found");

  // get the latest voted mint fee percent - depending on which currency user is paying with
  if (mint_fee_currency ==  symbol(FREEOS_CURRENCY_CODE, FREEOS_CURRENCY_PRECISION)) {
    mint_fee_percent = reward_iterator->mint_fee_percent;
  } else if (mint_fee_currency ==  symbol(XPR_CURRENCY_CODE, XPR_CURRENCY_PRECISION)) {
    mint_fee_percent = reward_iterator->mint_fee_percent_xpr;
  } else if (mint_fee_currency ==  symbol(XUSDC_CURRENCY_CODE, XUSDC_CURRENCY_PRECISION)) {
    mint_fee_percent = reward_iterator->mint_fee_percent_xusdc;
  }

  // we have to work in double rather asset for accuracy in division and because currencies (e.g. FREEOS, XPR and XUSDC) have different precisions
  double amount_in_units = mint_quantity.amount / ((double) POINT_UNIT_MULTIPLIER);
  double mintfee_in_freeos = amount_in_units * (mint_fee_percent / 100.0);

  // apply the minimum fee
  double mintfee_min = get_dparameter(name("mintfeemin"));
  if (mintfee_in_freeos < mintfee_min) {
    mintfee_in_freeos = mintfee_min;
  }

  // apply the currency conversion if necessary
  if (mint_fee_currency == symbol(FREEOS_CURRENCY_CODE, FREEOS_CURRENCY_PRECISION)) {
    mintfee_units = mintfee_in_freeos * FREEOS_UNIT_MULTIPLIER;
  } else {
    // conversion required

    // get the FREEOS exchange rate
    currencies_index currencies_table(get_self(), get_self().value);
    auto freeos_iterator = currencies_table.find(symbol(FREEOS_CURRENCY_CODE, FREEOS_CURRENCY_PRECISION).raw());
    check(freeos_iterator != currencies_table.end(), "FREEOS currency record not defined");
    double freeos_rate = freeos_iterator->usdrate;

    if (mint_fee_currency == symbol(XPR_CURRENCY_CODE, XPR_CURRENCY_PRECISION)) {
      // get the XPR exchange rate
      auto xpr_iterator = currencies_table.find(symbol(XPR_CURRENCY_CODE, XPR_CURRENCY_PRECISION).raw());
      check(xpr_iterator != currencies_table.end(), "XPR currency record not defined");
      double xpr_rate = xpr_iterator->usdrate;

      // do the conversion
      mintfee_amount = round(mintfee_in_freeos * freeos_rate / xpr_rate * XPR_UNIT_MULTIPLIER);
      mintfee_units = (int64_t) mintfee_amount;
    }

    if (mint_fee_currency == symbol(XUSDC_CURRENCY_CODE, XUSDC_CURRENCY_PRECISION)) {
      // get the XUSDC exchange rate
      auto xusdc_iterator = currencies_table.find(symbol(XUSDC_CURRENCY_CODE, XUSDC_CURRENCY_PRECISION).raw());
      check(xusdc_iterator != currencies_table.end(), "XUSDC currency record not defined");
      double xusdc_rate = xusdc_iterator->usdrate;

      // do the conversion
      mintfee_amount = round(mintfee_in_freeos * freeos_rate / xusdc_rate * XUSDC_UNIT_MULTIPLIER);
      mintfee_units = (int64_t) mintfee_amount;
    }
    
  }

  mintfee = asset(mintfee_units, mint_fee_currency);   // asset

  // DIAG
  // check(false, "mintfee_amount = " + to_string(mintfee_amount) + ", mintfee_units = " + to_string(mintfee_units) + ", mint fee = " + mintfee.to_string());
  
  return mintfee;
}

// This function is not called. If the mintfreeos transaction fails then all actions
// in the transaction are rolled back, which means there is no need to refund the mint fee.
// The code is reserved for future use


/**
 * Function looks up the user's credit record for the mint fee, and if it finds one, it transfers
 * the fee back to the user and deletes the credit record.
 * 
 * N.B. This function is not called. If the mintfreeos transaction fails then all actions
 * in the transaction are rolled back, which means there is no need to refund the mint fee.
 * The code is reserved for future use.
 * 
 * @param user the account name of the user who paid the fee
 * @param mint_fee_currency the currency that the user paid the mint fee in
 * 
 * @return The mint fee is being returned to the user.
 */
void freeosgov::refund_mintfee(name user, symbol mint_fee_currency) {

  credit_index credit_table(get_self(), user.value);
  auto credit_iterator = credit_table.find(mint_fee_currency.code().raw());

  if (credit_iterator == credit_table.end()) return;  // no credit record, so nothing to refund

  asset mintfee_paid = credit_iterator->balance;
  symbol mintfee_symbol = mintfee_paid.symbol;

  // look up the currencies table to get the contract name
  currencies_index currencies_table(get_self(), get_self().value);
  auto currency_iterator = currencies_table.find(mintfee_symbol.raw());

  if (currency_iterator == currencies_table.end()) return;  // unknown currency
  name currency_contract = currency_iterator->contract;

  // transfer the fee back to the user
  action transfer_action = action(
      permission_level{get_self(), "active"_n}, currency_contract,
      "transfer"_n,
      std::make_tuple(get_self(), user, mintfee_paid, string("refund of incorrect freeos mint fee")));

  transfer_action.send();

  // delete the credit record
  credit_table.erase(credit_iterator);
}


/**
 * Function `process_mint_fee` checks if the user has paid the correct mint fee, and if so, erases the credit
 * record and returns True to indicate that processing the mint fee was successful.
 * 
 * @param user the account name of the user who is minting the token
 * @param mint_quantity the amount of tokens to be minted
 * @param mint_fee_currency the symbol of the currency that the mint fee is paid in
 * 
 * @return A boolean value to indicate success or failure
 */
void freeosgov::process_mint_fee(name user, asset mint_quantity, symbol mint_fee_currency) {

  // calculate the mint fee
  asset mintfee = calculate_mint_fee(mint_quantity, mint_fee_currency);

  // has the user paid the mint fee, i.e. got credit?
  credit_index credit_table(get_self(), user.value);
  auto credit_iterator = credit_table.find(mint_fee_currency.code().raw());

  // check that mint fee credit record exists
  check(credit_iterator != credit_table.end(), "mint fee has not been paid");

  asset user_credit = credit_iterator->balance;

  // Check if the required mint-fee paid is the right amount - to within 1 smallest unit (to cope with rounding)
  check(abs(mintfee.amount - user_credit.amount) <= 1, "mint fee paid is incorrect, expected " + mintfee.to_string() + ", received " + user_credit.to_string());
  
  // erase the credit record
  credit_table.erase(credit_iterator);

}


/**
 * Function adjusts balances when minting FREEOS from POINTs.
 * It takes a user and an asset, and it decreases the user's balance of the asset and burns the asset.
 * 
 * @param user the account name of the user who is redeeming points
 * @param input_quantity the amount of POINTs the user is trying to convert
 */
void freeosgov::adjust_balances_from_points(const name user, const asset &input_quantity) {
  
  stats statstable(get_self(), input_quantity.symbol.code().raw());
  auto existing = statstable.find(input_quantity.symbol.code().raw());
  check(existing != statstable.end(), "token with symbol does not exist");
  const auto &st = *existing;

  // decrease user's balance of POINTs
  sub_balance(user, input_quantity);

  // burn the points
  check(st.supply.amount >= input_quantity.amount, "there are insufficient POINTs to burn");
  statstable.modify(st, same_payer, [&](auto &s) {
      s.supply -= input_quantity;
    });
}


/**
 * Function adjusts balances when minting FREEOS from POINTs.
 * It takes a user and an asset, and it decreases the user's balance of the asset and burns the asset.
 * 
 * @param user the account name of the user who is redeeming points
 * @param input_quantity the amount of POINTs the user is trying to convert
 */
void freeosgov::adjust_balances_from_freebi(const name user, const asset &input_quantity) {
  
  // check that we have a credit for the input FREEBI
  credit_index credits_table(get_self(), user.value);
  auto credit_iterator = credits_table.find(input_quantity.symbol.code().raw());

  // if there is not a credit record, proceed no further
  check(credit_iterator != credits_table.end(), "the FREEBI amount has not been paid");

  // check it is for the right amount
  asset freebi_paid = credit_iterator->balance;
  check(freebi_paid == input_quantity, "incorrect amount of freebi has been paid");

  // delete the FREEBI credit record
  credits_table.erase(credit_iterator);

  // burn the FREEBI amount
  string freebi_tokens_contract = get_parameter(name("freebitokens"));
  action retire_freebi_action = action(
    permission_level{get_self(), "active"_n}, name(freebi_tokens_contract),
    "retire"_n, std::make_tuple(input_quantity, string("burning FREEBI to mint FREEOS")));

  retire_freebi_action.send();
  
}


/**
 * Action checks that the user has paid the correct mint fee, then it issues the FREEOS tokens to the user
 * 
 * @param user the account name of the user who is minting
 * @param input_quantity the amount of POINTs or FREEBIs that the user is minting
 * @param mint_fee_currency the currency that the user has paid the mint fee in
 * @param use_airclaim_points If true, the user is using their Airclaim points to mint FREEOS.  If
 * false, the user is using POINTs or FREEBI to mint FREEOS.
 */
void freeosgov::mintfreeos(name user, const asset &input_quantity, symbol &mint_fee_currency, bool use_airclaim_points) {

  require_auth(user);

  // check that system is operational (masterswitch parameter set to "1")
  check(check_master_switch(), MSG_FREEOS_SYSTEM_NOT_AVAILABLE);

  uint32_t this_iteration = current_iteration();

  // is the system operational?
  check(this_iteration != 0, "The freeos system is not available at this time");

  check(input_quantity.is_valid(), "invalid quantity");
  check(input_quantity.amount > 0, "must mint a positive quantity");

  // check that the user has the appropriate POINTs balance to mint with
  // We don't need to do this for minting from FREEBI because the FREEBI is sent as a transfer credit
  if (input_quantity.symbol == POINT_CURRENCY_SYMBOL) {
    // get POINT balance
    asset points_balance = asset(0, POINT_CURRENCY_SYMBOL);  // default value of 0 POINTs
    accounts points_accounts_table(get_self(), user.value);
    auto points_iterator = points_accounts_table.find(POINT_CURRENCY_SYMBOL.code().raw());
    if (points_iterator != points_accounts_table.end()) {
      points_balance = points_iterator->balance;
    }

    check(points_balance.amount >= input_quantity.amount, "user has insufficient POINTs balance");
  }

  if (use_airclaim_points == false) {

    if (has_nft(user) == false) {   // users with an active NFT do not pay the mint fee
      // check whether user has paid correct mint fee, whether they have a credit record, adjust their mintfeefree allowance
      process_mint_fee(user, input_quantity, mint_fee_currency);
    }
 
  } else {
    // using airclaim mint-fee-free allowance
    // check mff balance, then decrease by the appropriate amount of POINTs minted

    // get the user's mff balance
    asset mff_balance = asset(0, POINT_CURRENCY_SYMBOL);  // default value
    mintfeefree_index mff_table(get_self(), user.value);
    auto mff_iterator = mff_table.begin();
    if (mff_iterator != mff_table.end()) {
      mff_balance = mff_iterator->balance;
    }

    // check whether the user has enough mff POINTs
    check(input_quantity.amount <= mff_balance.amount, "insufficient Airclaim points to mint the requested amount");

    // decrease the mff balance
    mff_table.modify(mff_iterator, get_self(),
                          [&](auto &mff) { mff.balance -= input_quantity; });
  }
  

  // different processing required for input_quantity currencies
  if (input_quantity.symbol == POINT_CURRENCY_SYMBOL) {
    adjust_balances_from_points(user, input_quantity);
  } else if (input_quantity.symbol == FREEBI_CURRENCY_SYMBOL) {
    adjust_balances_from_freebi(user, input_quantity);
  } else {
    check(false, "invalid currency for input quantity");
  }

  // Issue FREEOS
  asset exchangeable_amount = asset(input_quantity.amount, FREEOS_CURRENCY_SYMBOL);
  std::string memo = std::string("minting");

  // ask FREEOS contract to issue an equivalent amount of FREEOS tokens to the freeosgov account
  string freeos_tokens_contract = get_parameter(name("freeostokens"));
  action issue_action = action(
      permission_level{get_self(), "active"_n}, name(freeos_tokens_contract),
      "issue"_n, std::make_tuple(get_self(), exchangeable_amount, memo));

  issue_action.send();

  // transfer FREEOS tokens to the user
  action transfer_action = action(
      permission_level{get_self(), "active"_n}, name(freeos_tokens_contract),
      "transfer"_n,
      std::make_tuple(get_self(), user, exchangeable_amount, memo));

  transfer_action.send();

}


/**
 * Action withdraws all credits from the user's credit table and transfers the credit amount back to the user's account
 * 
 * @param user the account name of the user who is withdrawing their credits
 */
void freeosgov::withdraw(const name user) {
  action  transfer_action;
  asset   credit_amount;
  name    currency_contract;
  string  memo;

  require_auth(user);

  // check that system is operational (global masterswitch parameter set to "1")
  check(check_master_switch(), MSG_FREEOS_SYSTEM_NOT_AVAILABLE);

  // currencies table
  currencies_index currencies_table(get_self(), get_self().value);

  // credits table
  credit_index credits_table(get_self(), user.value);
  auto credit_iterator = credits_table.begin();

  while (credit_iterator != credits_table.end()) {
    // amount
    credit_amount = credit_iterator->balance;

    // get the currency contract
    string credit_code = credit_amount.symbol.code().to_string();
    if (credit_code == FREEOS_CURRENCY_CODE) {
      string freeos_tokens_contract = get_parameter(name("freeostokens"));
      currency_contract = name(freeos_tokens_contract);
    } else if (credit_code == FREEBI_CURRENCY_CODE) {
      string freebi_tokens_contract = get_parameter(name("freebitokens"));
      currency_contract = name(freebi_tokens_contract);
    } else {
      // look in the currencies table
      auto currency_iterator = currencies_table.find(credit_amount.symbol.raw());
      check(currency_iterator != currencies_table.end(), "currency record not found");
      currency_contract = currency_iterator->contract;
    }


    memo = string("withdrawal of credit: ") + credit_amount.to_string();

    action transfer_action = action(
      permission_level{get_self(), "active"_n}, name(currency_contract),
      "transfer"_n,
      std::make_tuple(get_self(), user, credit_amount, memo));

    transfer_action.send();

    // delete the credit record
    credit_iterator = credits_table.erase(credit_iterator);
  }

}


/**
 * Function finds the record for the iteration number, and if it doesn't exist, it creates it and initialises
 * it with the amount, otherwise it adds the amount to the existing record.
 * 
 * A deposit record for an iteration records how many POINTs are to be divided by the dividend contract to NFT holders.
 * 
 * @param iteration_number The iteration number of the deposit.
 * @param amount The amount of the deposit
 */
void freeosgov::record_deposit(uint32_t iteration_number, asset amount) {
  deposits_index deposits_table(get_self(), get_self().value);

  // find the record for the iteration
  auto deposit_iterator = deposits_table.find(iteration_number);

  if (deposit_iterator == deposits_table.end()) {
    // insert record and initialise
    deposits_table.emplace(get_self(), [&](auto &d) {
      d.iteration = iteration_number;
      d.accrued = amount;
    });
  } else {
    // modify record
    deposits_table.modify(deposit_iterator, get_self(),
                          [&](auto &d) { d.accrued += amount; });
  }
}


/**
 * Action deletes the deposit record for the iteration number passed in.
 * This is called by the dividend contract after successfully processing dividends for an iteration.
 * 
 * @param iteration_number The iteration number of the deposit record to be cleared.
 */
void freeosgov::depositclear(uint64_t iteration_number) {
  
  name freeosdiv_acct = name(get_parameter(name("freedaoacct")));
  require_auth(freeosdiv_acct);

  deposits_index deposits_table(get_self(), get_self().value);

  // find the record for the iteration
  auto deposit_iterator = deposits_table.find(iteration_number);

  check(deposit_iterator != deposits_table.end(),
        "a deposit record for the requested iteration does not exist");

  deposits_table.erase(deposit_iterator);
}

#ifdef BETA
/**
 * Action is called by the user to remove their account records for POINT and AIRCLAIM.
 * 
 * @param user the account name of the user who is removing
 * 
 * @return Nothing.
 */
void freeosgov::removetokens(const name &user) {
  require_auth(user);

  accounts accounts_table(get_self(), user.value);
  auto accounts_iterator = accounts_table.begin();

  while (accounts_iterator != accounts_table.end()) {
    accounts_iterator = accounts_table.erase(accounts_iterator);
  }
}
#endif


/**
 * When a user sends a token to the freeosgov contract, the freeosgov contract checks that the token is
 * from the valid token contract, and then records the amount of tokens recived as a credit record in the credit table.
 * 
 * @param user the account that sent the transfer
 * @param to the account that will receive the minted tokens
 * @param quantity the amount of the fee
 * @param memo "freeos mint fee"
 * 
 * @return The mintfee function is being returned.
 */
[[eosio::on_notify("*::transfer")]]    // was "eosio.token::transfer"
void freeosgov::mintfee(name user, name to, asset quantity, std::string memo) {
  if (memo == "freeos mint fee" || memo == "freeos mint credit") {

    if (user == get_self()) {
      return;
    }

    check(to == get_self(), "recipient of mint fee is incorrect");

    // check if the mint fee is in an acceptable currency
    symbol payment_symbol = quantity.symbol;
    currencies_index currencies_table(get_self(), get_self().value);
    auto currency_iterator = currencies_table.find(payment_symbol.raw());

    check(currency_iterator != currencies_table.end(), "payment is not in an accepted form of currency");

    // check that the mint fee was transferred from the official contract (don't take any wooden nickels)
    check(currency_iterator->contract == get_first_receiver(), "source of token is not valid");

    // record amount of fee in the credit table
    credit_index credit_table(get_self(), user.value);
    auto credit_iterator = credit_table.find(quantity.symbol.code().raw());

    // if there is already a credit record, proceed no further
    check(credit_iterator == credit_table.end(), "there is already a mint transaction in progress");

    // add the credit record
    credit_table.emplace(get_self(), [&](auto &c) {
      c.balance = quantity;
    });

  }

  /* DIAG - unused
  if (memo == "mint freebi to freeos") {
    if (user == get_self()) {
      return;
    }

    check(to == get_self(), "recipient of mint fee is incorrect");

    // check that FREEBI is from the right contract
    symbol freebi_symbol = symbol(FREEBI_CURRENCY_CODE, 4);
    check(quantity.symbol == freebi_symbol, "token symbol is not valid, expected FREEBI");
    string freebi_tokens_contract = get_parameter(name("freebitokens"));
    check(get_first_receiver() == name(freebi_tokens_contract), "FREEBI payment is invalid");

    // record amount of freebi in the credit table
    credit_index credit_table(get_self(), user.value);
    auto credit_iterator = credit_table.find(quantity.symbol.code().raw());

    // if there is already a credit record, proceed no further
    check(credit_iterator == credit_table.end(), "there is already a freebi->freeos mint transaction in progress");

    // add the credit record
    credit_table.emplace(get_self(), [&](auto &c) {
      c.balance = quantity;
    });
  } */

}

/** @} */ // end of points group
