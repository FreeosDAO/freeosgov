#pragma once
#include <eosio/eosio.hpp>
#include "eosio.proton.hpp"

using namespace eosio;
using namespace std;

namespace freedao {

// Andrew's code changes
//
// Survey global results table. For interpretations of particular rows search "survey.hpp".

struct[[ eosio::table("globalres"), eosio::contract("freeosgov") ]] globalres_struct {
    uint64_t p_key;
    double gresult;
    uint64_t primary_key() const { return p_key; }
};
using globalres_index = eosio::multi_index<"globalres"_n, globalres_struct>;
//===============

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

// POINTS ACCOUNTS
struct[[ eosio::table("accounts"), eosio::contract("freeosgov") ]] account {
  asset balance;

  uint64_t primary_key() const { return balance.symbol.code().raw(); }
};
typedef eosio::multi_index<"accounts"_n, account> accounts;


// currency stats
struct[[ eosio::table("stat"), eosio::contract("freeosgov") ]] currency_stats {
  asset supply;
  asset max_supply;
  asset conditional_supply;
  name issuer;

  uint64_t primary_key() const { return supply.symbol.code().raw(); }
};
typedef eosio::multi_index<"stat"_n, currency_stats> stats;


// transferers table - a whitelist of who can call the transfer function
struct[[ eosio::table("transferers"), eosio::contract("freeosgov") ]] transfer_whitelist {
  name account;

  uint64_t primary_key() const { return account.value; }
};
using transferers_index = eosio::multi_index<"transferers"_n, transfer_whitelist>;

// minters table - a whitelist of who can call the issue function
struct[[ eosio::table("minters"), eosio::contract("freeosgov") ]] minter_whitelist {
  name account;

  uint64_t primary_key() const { return account.value; }
};
using minters_index = eosio::multi_index<"minters"_n, minter_whitelist>;

// burners table - a whitelist of who can call the retire function
struct[[ eosio::table("burners"), eosio::contract("freeosgov") ]] burner_whitelist {
  name account;

  uint64_t primary_key() const { return account.value; }
};
using burners_index = eosio::multi_index<"burners"_n, burner_whitelist>;



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

// VOTE
// Running processing of vote responses
struct[[ eosio::table("vote"), eosio::contract("freeosgov") ]] vote {
    uint32_t iteration;
    uint32_t participants;
    double q1average;
    double q2average;
    double q3average;
    uint32_t q4choice1; // POOL
    uint32_t q4choice2; // BURN
    double q5average;
    uint32_t q6choice1;
    uint32_t q6choice2;
    uint32_t q6choice3;
    uint32_t q6choice4;
    uint32_t q6choice5;
    uint32_t q6choice6;
    uint64_t primary_key() const { return 0; } // return a constant to ensure a single-row table
};
using vote_index = eosio::multi_index<"vote"_n, vote>;




} // end of namespace freedao