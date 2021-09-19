//#pragma once
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include "freeosgov.hpp"
#include "constants.hpp"

using namespace eosio;
using namespace freedao;


// determine the user account type from the Proton verification table
string get_account_type(name user) {
  // default result
  string user_account_type = "e";

  // first determine which contract we consult - if we have set an alternative
  // contract then use that one
  name verification_contract = VERIFICATION_CONTRACT;

  // access the verification table
  usersinfo verification_table(name(verification_contract),
                               name(verification_contract).value);
  auto verification_iterator = verification_table.find(user.value);

  if (verification_iterator != verification_table.end()) {
    // record found, so default account_type is 'd', unless we find a
    // verification
    user_account_type = "d";

    auto kyc_prov = verification_iterator->kyc;

    for (int i = 0; i < kyc_prov.size(); i++) {
      size_t fn_pos = kyc_prov[i].kyc_level.find("firstname");
      size_t ln_pos = kyc_prov[i].kyc_level.find("lastname");

      if (verification_iterator->verified == true &&
          fn_pos != std::string::npos && ln_pos != std::string::npos) {
        user_account_type = "v";
        break;
      }
    }
  }

  return user_account_type;
}


void freeosgov::reguser(name user) {  // TODO: detect if the user has an existing record from the airclaim

  require_auth(user);

  check(current_iteration() != 0, "The freeos system is not yet available");

  // is the user already registered?
  // find the account in the user table
  users_index users_table(get_self(), user.value);
  auto user_iterator = users_table.begin();

  check(user_iterator == users_table.end(), "user is already registered");

  // check to see if they are already registered with the AirClaim // TODO: this is not required
  airclaim_users_index airclaim_users_table(AIRCLAIM_CONTRACT, user.value);
  auto airclaim_user_iterator = airclaim_users_table.begin();

  // determine account type
  string account_type = get_account_type(user);  // TODO

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