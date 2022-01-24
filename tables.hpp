#pragma once
#include <eosio/eosio.hpp>
#include "eosio.proton.hpp"

using namespace eosio;
using namespace std;

namespace freedao {

// Andrew's code changes
// Survey global results table. For interpretations of particular rows search "survey.hpp". TODO: remove table

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
uint16_t iteration;
uint32_t usercount;
uint32_t claimevents;
uint32_t participants;
asset cls;

uint64_t primary_key() const { return 0; } // return a constant to ensure a single-row table
};
using system_index = eosio::multi_index<"system"_n, system>;

// REWARDS
// rewards table
struct[[ eosio::table("rewards"), eosio::contract("freeosgov") ]] reward {

uint16_t  iteration;
asset     iteration_cls;
asset     iteration_issuance;
asset     participant_issuance;
uint32_t  participants;
double    issuance_rate;
double    mint_fee_percent;
double    locking_threshold;
bool      pool;
bool      burn;
bool      ratified;

uint64_t primary_key() const { return iteration; }
};
using rewards_index = eosio::multi_index<"rewards"_n, reward>;


// POINTS ACCOUNTS
struct[[ eosio::table("accounts"), eosio::contract("freeosgov") ]] account {
  asset balance;

  uint64_t primary_key() const { return balance.symbol.code().raw(); }
};
typedef eosio::multi_index<"accounts"_n, account> accounts;
typedef eosio::multi_index<"mintfeefree"_n, account> mintfeefree_index;


// currency stats
struct[[ eosio::table("stat"), eosio::contract("freeosgov") ]] currency_stats {
  asset supply;
  asset max_supply;
  asset conditional_supply; // not used. maintained for backwards compatibility with AirClaim POINTs token
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

// SURVEY
// Running processing of survey results
struct[[ eosio::table("surveyrecord"), eosio::contract("freeosgov") ]] survey_record {
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
using survey_index = eosio::multi_index<"surveyrecord"_n, survey_record>;

// VOTE
// Running processing of vote responses
struct[[ eosio::table("voterecord"), eosio::contract("freeosgov") ]] vote_record {
    uint32_t iteration;
    uint32_t participants;
    double q1average;   // issuance rate (0 - 100)
    double q2average;   // mint fee percent (6 - 30)
    double q3average;   // locking threshold - expressed as asset price
    uint32_t q4choice1; // POOL
    uint32_t q4choice2; // BURN
    double q5average;   // Reserve pool % to be released
    uint32_t q6choice1; // partner 1
    uint32_t q6choice2; // partner 2
    uint32_t q6choice3; // partner 3
    uint32_t q6choice4; // partner 4
    uint32_t q6choice5; // partner 5
    uint32_t q6choice6; // partner 6

    uint64_t primary_key() const { return 0; } // return a constant to ensure a single-row table
};
using vote_index = eosio::multi_index<"voterecord"_n, vote_record>;


// RATIFY
// Running processing of ratify responses
struct[[ eosio::table("ratifyrecord"), eosio::contract("freeosgov") ]] ratify_record {
    uint32_t iteration;
    uint32_t participants;
    uint32_t ratified;
    uint64_t primary_key() const { return 0; } // return a constant to ensure a single-row table
};
using ratify_index = eosio::multi_index<"ratifyrecord"_n, ratify_record>;


// EXCHANGERATE
// exchangerate table
struct[[ eosio::table("exchangerate"), eosio::contract("freeosgov") ]] price {
  double currentprice;
  double targetprice;

  uint64_t primary_key() const { return 0; } // return a constant (0 in this case) to ensure a single-row table
};
using exchange_index = eosio::multi_index<"exchangerate"_n, price>;

} // end of namespace freedao