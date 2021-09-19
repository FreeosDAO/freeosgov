#pragma once
#include <eosio/eosio.hpp>
#include "eosio.proton.hpp"

using namespace eosio;
using namespace std;

namespace freedao {


// SYSTEM
// system table
struct[[ eosio::table("system"), eosio::contract("freeosgov") ]] system {
time_point init;
uint32_t usercount;
uint32_t claimevents;
uint32_t votes;
asset cls;

uint64_t primary_key() const { return 0; } // return a constant to ensure a single-row table
};
using system_index = eosio::multi_index<"system"_n, system>;


// PARAMETERS
// parameters table
struct[[ eosio::table("parameters"), eosio::contract("freeosgov") ]] parameter {
name paramname;
string value;

uint64_t primary_key() const { return paramname.value; }
};
using parameters_index = eosio::multi_index<"parameters"_n, parameter>;


// USERS
// the registered user table
struct[[ eosio::table("users"), eosio::contract("freeosgov") ]] user {
  asset stake;                    // how many tokens staked
  string account_type;            // user's verification level
  uint32_t registered_iteration;  // when the user was registered
  uint32_t staked_iteration;      // the iteration in which the user staked their tokens
  uint32_t votes;                 // how many votes the user has made
  uint32_t issuances;             // total number of times the user has been issued with OPTIONs
  uint32_t last_issuance;         // the last iteration in which the user was issued with OPTIONs
  asset total_issuance_amount;    // accrued POINTs

  uint64_t primary_key() const { return 0; } // return a constant to ensure a single-row table
};
using users_index = eosio::multi_index<"users"_n, user>;


// AIRCLAIM_USERS
// the airclaim registered user table
struct[[ eosio::table("users"), eosio::contract("freeos") ]] airclaim_user {
  asset stake;                   // how many XPR tokens staked
  char account_type;             // user's verification level
  uint32_t registered_iteration; // when the user was registered
  uint32_t
      staked_iteration;   // the iteration in which the user staked their tokens
  uint32_t votes;         // how many votes the user has made
  uint32_t issuances;     // total number of times the user has been issued with
                          // OPTIONs
  uint32_t last_issuance; // the last iteration in which the user was issued
                          // with OPTIONs

  uint64_t primary_key() const { return stake.symbol.code().raw(); }
};
using airclaim_users_index = eosio::multi_index<"users"_n, airclaim_user>;

// USERSINFO
// Verification table - a mockup of the verification table on eosio.proton which is not available on the testnet
// This allows us to test in development.
// Used to determine a user's account_type. Taken from
// https://github.com/ProtonProtocol/proton.contracts/blob/master/contracts/eosio.proton/include/eosio.proton/eosio.proton.hpp
struct[[ eosio::table("usersinfo"), eosio::contract("eosio.proton") ]] userinfo {
  name acc;
  std::string name;
  std::string avatar;
  bool verified;
  uint64_t date;
  uint64_t verifiedon;
  eosio::name verifier;

  std::vector<eosio::name> raccs;
  std::vector<std::tuple<eosio::name, eosio::name>> aacts;
  std::vector<std::tuple<eosio::name, std::string>> ac;

  std::vector<kyc_prov> kyc;

  uint64_t primary_key() const { return acc.value; }
};
typedef eosio::multi_index<"usersinfo"_n, userinfo> usersinfo;


// PARTICIPATION
// survey, vote and ratification participation table
struct[[ eosio::table("svrs"), eosio::contract("freeosgov") ]] svr {
    uint32_t survey1;
    uint32_t survey2;
    uint32_t survey3;
    uint32_t survey4;
    uint32_t vote1;
    uint32_t vote2;
    uint32_t vote3;
    uint32_t vote4;
    uint32_t ratify1;
    uint32_t ratify2;
    uint32_t ratify3;
    uint32_t ratify4;

    uint64_t primary_key() const { return 0; } // return a constant to ensure a single-row table
};
using svr_index = eosio::multi_index<"svrs"_n, svr>;


// Running processing of survey results
struct[[ eosio::table("survey"), eosio::contract("freeosgov") ]] survey {
    uint32_t iteration;
    uint32_t participants;
    uint32_t q1choice1;
    uint32_t q1choice2;
    uint32_t q1choice3;
    double q2average;
    uint32_t q3choice1;
    uint32_t q3choice2;
    uint32_t q3choice3;
    double q4average;
    uint32_t q5choice1;
    uint32_t q5choice2;
    uint32_t q5choice3;
    uint32_t q5choice4;
    uint32_t q5choice5;
    uint32_t q5choice6;
    uint32_t q5choice7;
    uint32_t q5choice8;

    uint64_t primary_key() const { return 0; } // return a constant to ensure a single-row table
};
using survey_index = eosio::multi_index<"survey"_n, survey>;

} // end of namespace freedao