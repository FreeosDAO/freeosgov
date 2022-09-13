//#pragma once
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include "freeosgov.hpp"
#include "tables.hpp"

#include <cmath>

using namespace eosio;
using namespace freedao;
using namespace std;


// ACTION
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

void freeosgov::retire(const asset &quantity, const string &memo) {
  auto sym = quantity.symbol;
  check(sym.is_valid(), "invalid symbol name");
  check(memo.size() <= 256, "memo has more than 256 bytes");

  stats statstable(get_self(), sym.code().raw());
  auto existing = statstable.find(sym.code().raw());
  check(existing != statstable.end(), "token with symbol does not exist");
  const auto &st = *existing;

  check(quantity.is_valid(), "invalid quantity");
  check(quantity.amount > 0, "must retire positive quantity");

  check(quantity.symbol == st.supply.symbol, "symbol precision mismatch");

  statstable.modify(st, same_payer, [&](auto &s) { s.supply -= quantity; });

  sub_balance(st.issuer, quantity);
}

// Replacement for the transfer action - 'allocate' enforces a whitelist of
// those who can transfer
// ACTION
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

// ACTION
void freeosgov::unlock(const name &user) {
  require_auth(user);

  // user-activity-driven background process
  tick();

  // check that system is operational masterswitch parameter set to "1")
  check(check_master_switch(), MSG_FREEOS_SYSTEM_NOT_AVAILABLE);

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
  double locked_amount = locked_balance.amount / 10000.0;
  double percentage_applied = locked_amount * percentage;
  double adjusted_amount = ceil(percentage_applied); // rounding up to whole units
  uint64_t adjusted_units = adjusted_amount * 10000;

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


// Replacement for the issue action - 'mint' enforces a whitelist of who can issue OPTIONs
// ACTION
void freeosgov::mint(const name &minter, const name &to, const asset &quantity, const string &memo) {
  // check if the 'to' account is in the minter whitelist
  minters_index minters_table(get_self(), get_self().value);
  auto minter_iterator = minters_table.find(minter.value);

  check(minter_iterator != minters_table.end(), "the mint action is protected by minters whitelist");

  require_auth(minter);

  // if the 'to' user is in the minters table then call the issue function
  issue(to, quantity, memo);
}

// Replacement for the retire action - 'burn' enforces a whitelist of who can
// retire OPTIONs ACTION
void freeosgov::burn(const name &burner, const asset &quantity, const string &memo) {
  // check if the 'burner' account is in the burner whitelist
  burners_index burners_table(get_self(), get_self().value);
  auto burner_iterator = burners_table.find(burner.value);

  check(burner_iterator != burners_table.end(), "the burn action is protected by burners whitelist");

  require_auth(burner);

  // if the 'to' user is in the burners table then call the retire function
  retire(quantity, memo);
}

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


void freeosgov::sub_balance(const name &owner, const asset &value) {
  accounts from_acnts(get_self(), owner.value);

  const auto &from =
      from_acnts.get(value.symbol.code().raw(), "no balance object found");
  check(from.balance.amount >= value.amount, "overdrawn balance");

  from_acnts.modify(from, owner, [&](auto &a) { a.balance -= value; });
}

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

// convert POINTs for FREEBI
// ACTION
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
  auto points_balance_amount = 0;  // default value
  accounts points_accounts_table(get_self(), owner.value);
  auto points_iterator = points_accounts_table.find(POINT_CURRENCY_SYMBOL.code().raw());
  if (points_iterator != points_accounts_table.end()) {
    points_balance_amount = points_iterator->balance.amount;
  }
  check(points_balance_amount >= quantity.amount, "user has insufficient POINTs balance");

  stats statstable(get_self(), sym.code().raw());
  auto existing = statstable.find(sym.code().raw());
  check(existing != statstable.end(), "token with symbol does not exist");
  const auto &st = *existing;

  check(quantity.is_valid(), "invalid quantity");
  check(quantity.amount > 0, "must convert positive quantity");

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


asset freeosgov::calculate_mint_fee(name &user, asset &mint_quantity, symbol mint_fee_currency) {

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
  if (mint_fee_currency ==  symbol("FREEOS", 4)) {
    mint_fee_percent = reward_iterator->mint_fee_percent;
  } else if (mint_fee_currency ==  symbol("XPR", 4)) {
    mint_fee_percent = reward_iterator->mint_fee_percent_xpr;
  } else if (mint_fee_currency ==  symbol("XUSDC", 6)) {
    mint_fee_percent = reward_iterator->mint_fee_percent_xusdc;
  }

  // we have to work in double rather asset for accuracy in division and because currencies (e.g. FREEOS, XPR and XUSDC) have different precisions
  double amount_in_units = mint_quantity.amount / 10000.0;
  double mintfee_in_freeos = amount_in_units * (mint_fee_percent / 100.0);

  // apply the minimum fee
  double mintfee_min = get_dparameter(name("mintfeemin"));
  if (mintfee_in_freeos < mintfee_min) {
    mintfee_in_freeos = mintfee_min;
  }

  // apply the currency conversion if necessary
  if (mint_fee_currency == symbol("FREEOS", 4)) {
    mintfee_units = mintfee_in_freeos * 10000;
  } else {
    // conversion required

    // get the FREEOS exchange rate
    currencies_index currencies_table(get_self(), get_self().value);
    auto freeos_iterator = currencies_table.find(symbol("FREEOS", 4).raw());
    check(freeos_iterator != currencies_table.end(), "FREEOS currency record not defined");
    double freeos_rate = freeos_iterator->usdrate;

    if (mint_fee_currency == symbol("XPR",4)) {
      // get the XPR exchange rate
      auto xpr_iterator = currencies_table.find(symbol("XPR", 4).raw());
      check(xpr_iterator != currencies_table.end(), "XPR currency record not defined");
      double xpr_rate = xpr_iterator->usdrate;

      // do the conversion
      mintfee_amount = round(mintfee_in_freeos * freeos_rate / xpr_rate * 10000);
      mintfee_units = (int64_t) mintfee_amount;
    }

    if (mint_fee_currency == symbol("XUSDC",6)) {
      // get the XUSDC exchange rate
      auto xusdc_iterator = currencies_table.find(symbol("XUSDC", 6).raw());
      check(xusdc_iterator != currencies_table.end(), "XUSDC currency record not defined");
      double xusdc_rate = xusdc_iterator->usdrate;

      // do the conversion
      mintfee_amount = round(mintfee_in_freeos * freeos_rate / xusdc_rate * 1000000);
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

// function to check if the correct mint fee has been paid - returns true if mint fee has processed correctly
bool freeosgov::process_mint_fee(name user, asset mint_quantity, symbol mint_fee_currency) {

  bool mintfee_status;  // will be set to true if correct mint fee has been paid

  // calculate the mint fee
  asset mintfee = calculate_mint_fee(user, mint_quantity, mint_fee_currency);

  // has the user paid the mint fee, i.e. got credit?
  asset user_credit = asset(0, mint_fee_currency);  // default
  credit_index credit_table(get_self(), user.value);
  auto credit_iterator = credit_table.find(mint_fee_currency.code().raw());

  if (credit_iterator != credit_table.end()) {
    user_credit = credit_iterator->balance;
  }

  // Check if the mint-fee paid is the right amount
  if (mintfee == user_credit) {
    // correct amount
    // erase the credit record (if paid)
    if (credit_iterator != credit_table.end()) {
      credit_table.erase(credit_iterator);
    }
    
    mintfee_status = true;
  } else {
    // incorrect amount - refund the incorrect mint fee
    refund_mintfee(user, mint_fee_currency);
    mintfee_status = false;
  }

  return mintfee_status;
}

// adjust balances when minting from points
void freeosgov::adjust_balances_from_points(const name user, const asset &input_quantity) {
  
  stats statstable(get_self(), input_quantity.symbol.code().raw());
  auto existing = statstable.find(input_quantity.symbol.code().raw());
  check(existing != statstable.end(), "token with symbol does not exist");
  const auto &st = *existing;

  // decrease user's balance of POINTs
  sub_balance(user, input_quantity);

  // burn the points
  statstable.modify(st, same_payer, [&](auto &s) {
      s.supply -= input_quantity;
    });
}

// adjust balances when minting from freebi
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

// convert non-exchangeable currency for exchangeable currency
// ACTION: MINTFREEOS MINTFREEOS MINTFREEOS MINTFREEOS MINTFREEOS MINTFREEOS MINTFREEOS 
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
    auto points_balance_amount = 0;  // default value
    accounts points_accounts_table(get_self(), user.value);
    auto points_iterator = points_accounts_table.find(POINT_CURRENCY_SYMBOL.code().raw());
    if (points_iterator != points_accounts_table.end()) {
      points_balance_amount = points_iterator->balance.amount;
    }

    check(points_balance_amount >= input_quantity.amount, "user has insufficient POINTs balance");
  }

  if (use_airclaim_points == false) {
    // check whether user has paid correct mint fee, whether they have a credit record, adjust their mintfeefree allowance
    check(process_mint_fee(user, input_quantity, mint_fee_currency) == true, "incorrect mint fee has been paid");
  } else {
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

// ACTION
// withdraw credits
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
    if (credit_code == "FREEOS") {
      string freeos_tokens_contract = get_parameter(name("freebitokens"));
      currency_contract = name(freeos_tokens_contract);
    } else if (credit_code == "FREEBI") {
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

// record a deposit to the freedao account
void freeosgov::record_deposit(uint64_t iteration_number, asset amount) {
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

// action to clear (remove) a deposit record from the deposit table
// ACTION
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



// mint fee confirmation
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

  if (memo == "mint freebi to freeos") {
    if (user == get_self()) {
      return;
    }

    check(to == get_self(), "recipient of mint fee is incorrect");

    // check that FREEBI is from the right contract
    symbol freebi_symbol = symbol("FREEBI", 4);
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
  }
}