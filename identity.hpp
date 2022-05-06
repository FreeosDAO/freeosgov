#pragma once
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include "freeosgov.hpp"
#include "constants.hpp"
#include "config.hpp"
#include "tables.hpp"

#include <stdlib.h>

using namespace eosio;
using namespace freedao;

// user's last active iteration
uint32_t freeosgov::user_last_active_iteration(name user) {
  // fetch the user lifespan parameter
  string slifespan = get_parameter(name("userlifespan"));
  check(slifespan != "", "user lifespan parameter is not defined");
  uint32_t ilifespan = abs(stoi(slifespan));

  // get the user record
  users_index users_table(get_self(), user.value);
  auto user_iterator = users_table.begin();
  check(user_iterator != users_table.end(), "user registration record is not defined");

  return user_iterator->registered_iteration + ilifespan - 1;
}

// is the user within the allotted 'lifespan' ?
bool freeosgov::is_user_alive(name user) {
  uint32_t user_last_iteration = user_last_active_iteration(user);

  // is user alive? if current_iteration > (user's registered_iteration + lifespan - 1) then user has exceeeded lifespan
  return current_iteration() <= user_last_iteration ? true : false;
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

// add user CLS contribution
asset freeosgov::calculate_user_cls_addition() {
  // get parameters
  parameters_index parameters_table(get_self(), get_self().value);

  // user cls contribution
  int64_t uclsamount;
  auto uclsamount_iterator = parameters_table.find(name("uclsamount").value);
  if (uclsamount_iterator != parameters_table.end()) {
    uclsamount = stoi(uclsamount_iterator->value);
  } else {
    uclsamount = UCLSAMOUNT;  // hard floor constant
  }

  // get double parameters
  dparameters_index dparameters_table(get_self(), get_self().value);

  // daoshare
  double daoshare;
  auto daoshare_iterator = dparameters_table.find(name("daoshare").value);
  if (daoshare_iterator != dparameters_table.end()) {
    daoshare = daoshare_iterator->value;
  } else {
    daoshare = DAOSHARE;  // hard floor constant
  }

  // partnershare
  double partnershare;
  auto partnershare_iterator = dparameters_table.find(name("partnershare").value);
  if (partnershare_iterator != dparameters_table.end()) {
    partnershare = partnershare_iterator->value;
  } else {
    partnershare = PARTNERSHARE;  // hard floor constant
  }

  // calculate the total asset for user CLS
  int64_t user_cls_amount = uclsamount * (1.0 + daoshare + partnershare);
  asset ucls = asset(user_cls_amount * 10000, POINT_CURRENCY_SYMBOL);

  return ucls;
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
  mintfeefree_index mintfeefree_table(get_self(), user.value);

  auto points_iterator = accounts_table.find(symbol_code(POINT_CURRENCY_CODE).raw());
  if (points_iterator != accounts_table.end()) {    
      // get and store the POINTs balance
      asset points_balance = points_iterator->balance;

      // store in the mint_fee_free table
      auto mintfeefree_iterator = mintfeefree_table.begin();

      if (mintfeefree_iterator == mintfeefree_table.end()) {
        // emplace
        mintfeefree_table.emplace(get_self(), [&](auto &m) {
          m.balance = points_balance;
        });
      } else {
        // modify - should not be necessary to modify, but include this code in the event of migrations, etc
        mintfeefree_table.modify(mintfeefree_iterator, get_self(), [&](auto &m) {
          m.balance = points_balance;
        });
      } 
  }

  // check if the user has an AIRKEY - in which case they get a mint-fee-free waiver of mint-fee on their POINTs  
  auto airkey_iterator = accounts_table.find(symbol_code(AIRKEY_CURRENCY_CODE).raw());
  if (airkey_iterator != accounts_table.end()) {
    // store mint-fee-free allowance in the mint_fee_free table
    asset airkey_allowance = asset(AIRKEY_MINT_FEE_FREE_ALLOWANCE * 10000, POINT_CURRENCY_SYMBOL);

    auto mintfeefree_iterator = mintfeefree_table.begin();

    if (mintfeefree_iterator == mintfeefree_table.end()) {
      // emplace
      mintfeefree_table.emplace(get_self(), [&](auto &m) {
        m.balance = airkey_allowance;
      });
    } else {
      mintfeefree_table.modify(mintfeefree_iterator, get_self(), [&](auto &m) {
        m.balance += airkey_allowance;
      });
    } 
  }

  // determine account type
  string account_type = get_account_type(user);  // TODO

  // get the current iteration
  uint16_t iteration = current_iteration();

  // TODO: staking code

  // add record to the users table
  users_table.emplace(get_self(), [&](auto &user) {
    user.stake = asset(0, XPR_CURRENCY_SYMBOL);
    user.account_type = account_type;
    user.registered_iteration = iteration;
    user.staked_iteration = iteration; // TODO
    user.total_issuance_amount = asset(0, POINT_CURRENCY_SYMBOL);
  });

  // update the system record - number of users and CLS
  asset ucls = calculate_user_cls_addition();

  system_index system_table(get_self(), get_self().value);
  auto system_iterator = system_table.begin();
  if (system_iterator == system_table.end()) {
    // emplace
    system_table.emplace(
        get_self(), [&](auto &sys) {
          sys.usercount = 1;

          // update the CLS if a verified user
          if (is_user_verified(user)) {
            sys.cls = ucls;
          }
          
        });
  } else {
    // modify
    system_table.modify(system_iterator, get_self(), [&](auto &sys) {
      sys.usercount += 1;

      // update the CLS if a verified user
      if (is_user_verified(user)) {
        sys.cls += ucls;
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
  users_table.modify(user_iterator, get_self(), [&](auto &u) {
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
    asset ucls = calculate_user_cls_addition();

    system_table.modify(system_iterator, get_self(), [&](auto &sys) {
        sys.cls += ucls;
    });
  }

}


// is user registered?
bool freeosgov::is_registered(name user) {
  users_index users_table(get_self(), user.value);
  auto user_iterator = users_table.begin();

  return (user_iterator != users_table.end()) ? true : false;
}


bool freeosgov::is_user_verified(name user) {
  bool verified = false;

  // get the user record
  users_index users_table(get_self(), user.value);
  auto user_iterator = users_table.begin();
  check(user_iterator != users_table.end(), "user is not registered");
  string account_type = user_iterator->account_type;

  if (account_type == "v" || account_type == "b" || account_type == "c") {
    verified= true;
  }

  return verified;
}