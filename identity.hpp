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


/** @defgroup identity Identity
 *  These Actions and functions are related to the personal attributes of each participant.
 *  @{
 */


/**
 * Function checks the `nft_table` in the `dividend` contract to see if the user has an active (unlocked) NFT
 * 
 * @param user the account name of the user
 * 
 * @return A boolean value indicating whether the user has an active NFT
 */
bool freeosgov::has_nft(name user) {
  bool nft_status = false;

  // check the dividend contract
  name dividend_contract = name(get_parameter(name("freedaoacct")));

  nft_table nfts(dividend_contract, dividend_contract.value);
  auto account_index = nfts.get_index<"active"_n>();
  auto nft_iterator = account_index.find(user.value);

  if (nft_iterator != account_index.end()) {  // if NFT record found
    nft_status = true;
  }

  return nft_status;
}


/**
 * Function returns the iteration number after which the user will be deactivated
 * 
 * @param user the user account name
 * 
 * @return The last iteration that the user is active.
 */
uint32_t freeosgov::user_last_active_iteration(name user) {
  // fetch the user lifespan parameter
  string slifespan = get_parameter(name("userlifespan"));
  check(slifespan != "", "user lifespan parameter is not defined");
  uint32_t ilifespan = abs(stoi(slifespan));

  // get the user record
  participants_index participants_table(get_self(), user.value);
  auto participant_iterator = participants_table.begin();
  check(participant_iterator != participants_table.end(), "user registration record is not defined");

  return participant_iterator->registered_iteration + ilifespan - 1;
}


/**
 * If the current iteration is less than or equal to the user's last active iteration, then the user is
 * alive
 * 
 * @param user: the user's account name
 * 
 * @return A boolean value
 */
bool freeosgov::is_user_alive(name user) {
  uint32_t user_last_iteration = user_last_active_iteration(user);

  // is user alive? if current_iteration > (user's registered_iteration + lifespan - 1) then user has exceeeded lifespan
  return current_iteration() <= user_last_iteration ? true : false;
}


/**
 * Function checks the user's verification status and returns a string indicating the user's account type, as follows:
 * "d" indicates an account created via the Proton wallet,
 * "e" indicates an account created programmatically, i.e. not via a Proton process
 * "v" indicates that the user has completed the Proton KYC process.
 * 
 * The above is determined by examining the user entry (if it exists) in the eosio.proton::usersinfo table
 * 
 * @param user the account name of the user we are checking
 * 
 * @return The account type of the user.
 */
string freeosgov::get_account_type(name user) {
  // default result
  string user_account_type = "e";

  // first determine which contract we consult - if we have set an alternative
  // contract then use that one
  name verification_contract = name(get_parameter(name("verifyacct")));

  // access the verification table
  usersinfo verification_table(verification_contract,
                               verification_contract.value);
  auto verification_iterator = verification_table.find(user.value);

  if (verification_iterator != verification_table.end()) {
    // record found, so default account_type is 'd', unless we find a
    // verification
    user_account_type = "d";

    // New requirement, as of v0.10.0 - KYC'ed accounts must also be name verified (verified == true)
    if (verification_iterator->verified) {
      auto kyc_prov = verification_iterator->kyc;

      for (int i = 0; i < kyc_prov.size(); i++) {
        size_t fn_pos = kyc_prov[i].kyc_level.find("firstname");
        size_t ln_pos = kyc_prov[i].kyc_level.find("lastname");

        if (fn_pos != std::string::npos && ln_pos != std::string::npos) {
          user_account_type = "v";
          break;
        }
      }
    };


  }

  return user_account_type;
}


/**
 * Function calculates the amount by which the Conditionally Limited Supply (CLS) will be increased on a user registration
 * 
 * @return asset: The number of POINTs to add to the CLS 
 */
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
  asset ucls = asset(user_cls_amount * POINT_UNIT_MULTIPLIER, POINT_CURRENCY_SYMBOL);

  return ucls;
}


/**
 * The `reguser` action is called by a user to register with the Freeos system.
 * 
 * The function:
 * 1. creates an appropriately populated record in the participants table,
 * 2. creates and populates the mff (mint-fee-free) record with the user's POINTs balance,
 * 3. updates the CLS with a contribution for the user
 * 
 * @param user the user's account name
 */
void freeosgov::reguser(name user) {  // TODO: detect if the user has an existing record from the airclaim

  require_auth(user);

  // check that system is operational (masterswitch parameter set to "1")
  check(check_master_switch(), MSG_FREEOS_SYSTEM_NOT_AVAILABLE);

  // get the current iteration
  uint32_t iteration = current_iteration();

  // check(iteration != 0, "The freeos system is not yet available");

  // is the user already registered?
  // find the account in the participants table
  participants_index participants_table(get_self(), user.value);
  auto participant_iterator = participants_table.begin();

  check(participant_iterator == participants_table.end(), "user is already registered");

  // determine account type
  string account_type = get_account_type(user);
  check(account_type == "v" || account_type == "b" || account_type == "c" || has_nft(user), "please complete kyc before registering");

  // capture the user's POINTs balance - as these POINTs will be mint-fee-free
  asset liquid_points = asset(0, POINT_CURRENCY_SYMBOL);  // default=0 if POINTs balance record not found
  accounts accounts_table(get_self(), user.value);
  auto points_iterator = accounts_table.find(symbol_code(POINT_CURRENCY_CODE).raw());
  if (points_iterator != accounts_table.end()) {
    liquid_points = points_iterator->balance;
  }

  // also include the locked POINTs balance
  asset locked_points = asset(0, POINT_CURRENCY_SYMBOL); // default=0 if locked POINTs balance record not found
  lockaccounts locked_accounts_table(get_self(), user.value);
  auto locked_points_iterator = locked_accounts_table.find(symbol_code(POINT_CURRENCY_CODE).raw());
  if (locked_points_iterator != locked_accounts_table.end()) {
    locked_points = locked_points_iterator->balance;
  }

  // determine if the user has an AIRKEY
  asset airkey_allowance = asset(0, POINT_CURRENCY_SYMBOL); // default=0 if no AIRKEY
  auto airkey_iterator = accounts_table.find(symbol_code(AIRKEY_CURRENCY_CODE).raw());
  if (airkey_iterator != accounts_table.end()) {
    airkey_allowance = asset(AIRKEY_MINT_FEE_FREE_ALLOWANCE * POINT_UNIT_MULTIPLIER, POINT_CURRENCY_SYMBOL);
  }

  asset mintfeefree_allowance = liquid_points + locked_points + airkey_allowance;

  // store the mint-fee-free allowance
  if (mintfeefree_allowance.amount > 0) {   

      // store in the mint_fee_free table
      mintfeefree_index mintfeefree_table(get_self(), user.value); 
      auto mintfeefree_iterator = mintfeefree_table.begin();

      if (mintfeefree_iterator == mintfeefree_table.end()) {
        // emplace
        mintfeefree_table.emplace(get_self(), [&](auto &m) {
          m.balance = mintfeefree_allowance;
        });
      } else {
        // modify - should not be necessary to modify, but include this code in the event of migrations, etc
        mintfeefree_table.modify(mintfeefree_iterator, get_self(), [&](auto &m) {
          m.balance = mintfeefree_allowance;
        });
      } 
  }

  // add record to the participants table
  participants_table.emplace(get_self(), [&](auto &participant) {
    participant.account_type = account_type;
    participant.registered_iteration = iteration;
    participant.total_issuance_amount = asset(0, POINT_CURRENCY_SYMBOL);
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


/**
 * The reregister function is called by a user to update their account_type in their participant record
 * 
 * @param user the account name of the user who is reregistering
 */
void freeosgov::reregister(name user) {
  require_auth(user);

  // check that system is operational (masterswitch parameter set to "1")
  check(check_master_switch(), MSG_FREEOS_SYSTEM_NOT_AVAILABLE);

  // original verified user status
  bool verified_before = is_user_verified(user);

  // get the current iteration
  check(current_iteration() != 0, "The freeos system is not yet available");

  // set the account type
  participants_index participants_table(get_self(), user.value);
  auto participant_iterator = participants_table.begin();

  // check if the user has a user registration record
  check(participant_iterator != participants_table.end(), "user is not registered with freeos");

  // get the account type
  string account_type = get_account_type(user);

  // set the participant account type
  participants_table.modify(participant_iterator, get_self(), [&](auto &participant) {
    participant.account_type = account_type;
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


/**
 * Function checks if the user is registered in the participants table
 * 
 * @param user The account name of the user to check.
 * 
 * @return A boolean value.
 */
bool freeosgov::is_registered(name user) {

  participants_index participants_table(get_self(), user.value);
  auto participant_iterator = participants_table.begin();

  return (participant_iterator != participants_table.end()) ? true : false;
}


/**
 * If the user is a verified user, or if the user has an NFT, then the user is verified
 * 
 * @param user the account name of the user
 * 
 * @return A boolean value.
 */
bool freeosgov::is_user_verified(name user) {
  bool verified = false;

  // get the participant record
  participants_index participants_table(get_self(), user.value);
  auto participant_iterator = participants_table.begin();
  check(participant_iterator != participants_table.end(), "user is not registered");
  string account_type = participant_iterator->account_type;

  if (account_type == "v" || account_type == "b" || account_type == "c") {
    verified= true;
  }

  if (has_nft(user)) {
    verified = true;
  }

  return verified;
}

/** @} */ // end of identity group