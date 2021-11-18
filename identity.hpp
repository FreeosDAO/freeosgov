#pragma once
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include "freeosgov.hpp"
#include "constants.hpp"
#include "config.hpp"
#include "tables.hpp"

using namespace eosio;
using namespace freedao;


// is the user within the allotted 'lifespan' ?
bool freeosgov::is_user_active(name user) {

  // default return value
  bool user_valid = true;

  // find the user lifespan parameter
  string slifespan = get_parameter(name("userlifespan"));
  uint32_t ilifespan = stoi(slifespan);

  // get the user record
  users_index users_table(get_self(), user.value);
  auto user_iterator = users_table.begin();

  check(user_iterator != users_table.end(), "user record is not defined");

  // if current_iteration > user's registered_iteration + lifespan then user has exceeeded lifespan
  if (current_iteration() > (user_iterator->registered_iteration + ilifespan)) {
    user_valid = false;
  }

  return user_valid;
}

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

// ACTION
void freeosgov::reguser(name user) {  // TODO: detect if the user has an existing record from the airclaim

  require_auth(user);

  check(current_iteration() != 0, "The freeos system is not yet available");

  // is the user already registered?
  // find the account in the user table
  users_index users_table(get_self(), user.value);
  auto user_iterator = users_table.begin();

  check(user_iterator == users_table.end(), "user is already registered");

  // capture their POINTs balance - as these POINTs will be mint-fee-free
  accounts accounts_table(get_self(), user.value);
  auto points_iterator = accounts_table.find(symbol_code(POINT_CURRENCY_CODE).raw());
  if (points_iterator != accounts_table.end()) {    
      // get and store the POINTs balance
      asset points_balance = points_iterator->balance;

      // store in the mint_fee_free table
      mintfeefree_index mintfeefree_table(get_self(), user.value);
      mintfeefree_table.emplace(get_self(), [&](auto &m) {
        m.balance = points_balance;
      });
  }

  // determine account type
  string account_type = get_account_type(user);  // TODO

  // get the current iteration
  uint16_t iteration = current_iteration();

  // TODO: staking code

  // add record to the users table
  users_table.emplace(get_self(), [&](auto &user) {
    user.stake = asset(0, STAKE_CURRENCY_SYMBOL);
    user.account_type = account_type;
    user.registered_iteration = iteration;
    user.staked_iteration = iteration; // TODO
    user.total_issuance_amount = asset(0, POINT_CURRENCY_SYMBOL);
  });

  // update the system record - number of users and CLS
  system_index system_table(get_self(), get_self().value);
  auto system_iterator = system_table.begin();
  if (system_iterator == system_table.end()) {
    // emplace
    system_table.emplace(
        get_self(), [&](auto &sys) {
          sys.usercount = 1;

          // update the CLS if a verified user
          if (is_user_verified(user)) {
            sys.cls = UCLS; // the CLS for the first verified user
            sys.cls += PARTNER_CLS_ADDITION; // add to CLS for the partners
          }
          
        });
  } else {
    // modify
    system_table.modify(system_iterator, _self, [&](auto &sys) {
      sys.usercount += 1;

      // update the CLS if a verified user
      if (is_user_verified(user)) {
        sys.cls += UCLS; // add to the CLS for the verified user
        sys.cls += PARTNER_CLS_ADDITION; // add to CLS for the partners
      }
    });
  }

  // TODO: delete the record from the old users table if they are already registered with the AirClaim
  // TODO: gov 'users' table to be renamed 'register'

}


// ACTION
void freeosgov::reregister(name user) {
  require_auth(user);

  // original verified user status
  bool verified_before = is_user_verified(user);

  // get the current iteration
  check(current_iteration() != 0, "The freeos system is not yet available");

  // set the account type
  users_index users_table(get_self(), user.value);
  auto user_iterator = users_table.begin();

  // check if the user has a user registration record
  check(user_iterator != users_table.end(), "user is not registered with freeos");

  // get the account type
  string account_type = get_account_type(user);

  // set the user account type
  users_table.modify(user_iterator, _self, [&](auto &u) {
    u.account_type = account_type;
  });

  // new verified user status
  bool verified_after = is_user_verified(user);

  // update the CLS if a user has become verified
  if (verified_before == false && verified_after == true) {
    system_index system_table(get_self(), get_self().value);
    auto system_iterator = system_table.begin();
    check(system_iterator != system_table.end(), "system record is undefined");

    // modify cls
    system_table.modify(system_iterator, _self, [&](auto &sys) {
        sys.cls += UCLS; // add to the CLS for the verified user
        sys.cls += PARTNER_CLS_ADDITION; // add to CLS for the partners
    });
  }

}


// is user registered?
bool freeosgov::is_registered(name user) {
  users_index users_table(get_self(), user.value);
  auto user_iterator = users_table.begin();

  if (user_iterator == users_table.end()) {
    return false;
  } else {
    return true;
  }
}


bool freeosgov::is_user_verified(name user) {
  bool verified = false;

  // get the user record
  users_index users_table(get_self(), user.value);
  auto user_iterator = users_table.begin();
  check(user_iterator != users_table.end(), "user is not registered");
  string account_type = user_iterator->account_type;

  // TODO: remove "e" from condition below
  if (account_type == "v" || account_type == "b" || account_type == "c" || account_type == "e") {
    verified= true;
  }

  return verified;
}