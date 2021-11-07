//#pragma once
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include "freeosgov.hpp"
#include "tables.hpp"

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
    s.conditional_supply.symbol = maximum_supply.symbol;
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
void freeosgov::mintfreeby(const name &owner, const asset &quantity) {
  require_auth(owner);

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
    s.conditional_supply -= quantity;
  });

  // decrease owner's balance of non-exchangeable tokens
  sub_balance(owner, quantity);

  // Issue exchangeable tokens
  asset exchangeable_amount = asset(quantity.amount, FREEBY_CURRENCY_SYMBOL);
  std::string memo = std::string("conversion");

  // ask FREEBY contract to issue an equivalent amount of FREEBY tokens to the freeosgov account
  action issue_action = action(
      permission_level{get_self(), "active"_n}, name(freeby_acct),
      "issue"_n, std::make_tuple(get_self(), exchangeable_amount, memo));

  issue_action.send();

  // transfer FREEBY tokens to the owner
  action transfer_action = action(
      permission_level{get_self(), "active"_n}, name(freeby_acct),
      "transfer"_n,
      std::make_tuple(get_self(), owner, exchangeable_amount, memo));

  transfer_action.send();
}
