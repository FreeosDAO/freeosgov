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

// convert non-exchangeable currency for exchangeable currency
// ACTION
void freeosgov::mintfreebi(const name &owner, const asset &quantity) {
  require_auth(owner);

  // is the 'owner' user verified?
  check(is_user_verified(owner), "minting is restricted to verified users");

  auto sym = quantity.symbol;
  check(sym == POINT_CURRENCY_SYMBOL, "invalid symbol name");

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

  // ask FREEBY contract to issue an equivalent amount of FREEBY tokens to the freeosgov account
  action issue_action = action(
      permission_level{get_self(), "active"_n}, name(freebi_acct),
      "issue"_n, std::make_tuple(get_self(), exchangeable_amount, memo));

  issue_action.send();

  // transfer FREEBY tokens to the owner
  action transfer_action = action(
      permission_level{get_self(), "active"_n}, name(freebi_acct),
      "transfer"_n,
      std::make_tuple(get_self(), owner, exchangeable_amount, memo));

  transfer_action.send();
}


asset freeosgov::calculate_mint_fee(name &user, asset &mint_quantity, symbol mint_fee_currency) {

  asset points_subject_to_fee = asset(0, POINT_CURRENCY_SYMBOL);  // default value
  asset mintfeefree_allowance = asset(0, POINT_CURRENCY_SYMBOL);  // default value
  asset mintfee = asset(0, XPR_CURRENCY_SYMBOL);                  // default value - TODO: set for different currencies
  // express the mint quantity as an equivalent number of POINTs
  asset mint_quantity_points = asset(mint_quantity.amount, POINT_CURRENCY_SYMBOL);

  // adjust the requested quantity as mint fee may be fully or partially covered by the mint-fee-free allowance
  mintfeefree_index mintfeefree_table(get_self(), user.value);
  auto mintfeefree_iterator = mintfeefree_table.begin();
  if (mintfeefree_iterator != mintfeefree_table.end()) {

    mintfeefree_allowance = mintfeefree_iterator->balance;

    if (mintfeefree_allowance.amount > mint_quantity.amount) {
      // user can cover the mint fee requirement with their allowance

      // decrease the minfeefree allowance accordingly
      mintfeefree_table.modify(mintfeefree_iterator, same_payer, [&](auto &m) {
        m.balance -= mint_quantity_points;
      });

      points_subject_to_fee = asset(0, POINT_CURRENCY_SYMBOL);
    } else {
      // user can cover SOME of their mint fee with the allowance
      points_subject_to_fee = mint_quantity_points - mintfeefree_allowance;

      // the mintfeefree allowance is used up, so delete the record to save RAM
      mintfeefree_table.erase(mintfeefree_iterator);
    }
  } else {
    // user has no mintfeefree allowance, the entire amount of points is subject to the mint fee
    points_subject_to_fee = mint_quantity_points;
  }
  
  if (points_subject_to_fee.amount > 0) {
    mintfee = asset(1 * 10000, mint_fee_currency);
  } else {
    mintfee = asset(0, mint_fee_currency);
  }

  return mintfee;
}


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
  action retire_freebi_action = action(
    permission_level{get_self(), "active"_n}, name(freebi_acct),
    "retire"_n, std::make_tuple(input_quantity, string("burning FREEBI to mint FREEOS")));

  retire_freebi_action.send();
}

// convert non-exchangeable currency for exchangeable currency
// ACTION: MINTFREEOS MINTFREEOS MINTFREEOS MINTFREEOS MINTFREEOS MINTFREEOS MINTFREEOS 
void freeosgov::mintfreeos(name user, const asset &input_quantity, symbol &mint_fee_currency) {

  require_auth(user);

  check(input_quantity.is_valid(), "invalid quantity");
  check(input_quantity.amount > 0, "must mint a positive quantity");

  // check whether user has paid correct mint fee, whether they have a credit record, adjust their mintfeefree allowance
  check(process_mint_fee(user, input_quantity, mint_fee_currency) == true, "incorrect mint fee has been paid");

  // different processing required for input_quantity currencies
  symbol input_currency_symbol = input_quantity.symbol;
  if (input_currency_symbol == POINT_CURRENCY_SYMBOL) {
    adjust_balances_from_points(user, input_quantity);
  } else if (input_currency_symbol == FREEBI_CURRENCY_SYMBOL) {
    adjust_balances_from_freebi(user, input_quantity);
  } else {
    check(false, "invalid currency for input quantity");
  }

  // Issue FREEOS
  asset exchangeable_amount = asset(input_quantity.amount, FREEOS_CURRENCY_SYMBOL);
  std::string memo = std::string("minting");

  // ask FREEOS contract to issue an equivalent amount of FREEOS tokens to the freeosgov account
  action issue_action = action(
      permission_level{get_self(), "active"_n}, name(freeos_acct),
      "issue"_n, std::make_tuple(get_self(), exchangeable_amount, memo));

  issue_action.send();

  // transfer FREEOS tokens to the user
  action transfer_action = action(
      permission_level{get_self(), "active"_n}, name(freeos_acct),
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
      currency_contract = name(freeos_acct);
    } else if (credit_code == "FREEBI") {
      currency_contract = name(freebi_acct);
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


// mint fee confirmation
[[eosio::on_notify("*::transfer")]]    // was "eosio.token::transfer"
void freeosgov::mintfee(name user, name to, asset quantity, std::string memo) {
  if (memo == "freeos mint fee") {

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
    check(get_first_receiver() == name(freebi_acct), "FREEBI payment is invalid");

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