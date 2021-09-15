//#pragma once
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include "freeosgov.hpp"
#include "constants.hpp"

using namespace eosio;
using namespace freedao;


void freeosgov::reguser(name user) {  // TODO: detect if the user has an existing record from the airclaim

  require_auth(user);

  // is the user already registered?
  // find the account in the user table
  users_index users_table(get_self(), user.value);
  auto user_iterator = users_table.begin();

  check(user_iterator == users_table.end(), "user is already registered");

  // determine account type
  string account_type = "e";  // TODO

  // update the user count in the 'system' record
  uint32_t number_of_users;

  system_index system_table(get_self(), get_self().value);
  auto system_iterator = system_table.begin();
  if (system_iterator == system_table.end()) {
    // emplace
    system_table.emplace(
        get_self(), [&](auto &sys) { sys.usercount = number_of_users = 1; });
  } else {
    // modify
    system_table.modify(system_iterator, _self, [&](auto &sys) {
      sys.usercount = number_of_users = sys.usercount + 1;
    });
  }

  // get the current iteration
  uint16_t iteration = current_iteration();

  // TODO: staking code

  // register the user
  users_table.emplace(get_self(), [&](auto &user) {
    user.stake = asset(0, STAKE_CURRENCY_SYMBOL);
    user.account_type = account_type;
    user.registered_iteration = iteration;
    user.staked_iteration = iteration; // TODO
    user.total_issuance_amount = asset(0, POINT_CURRENCY_SYMBOL);
  });

}