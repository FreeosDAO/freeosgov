#pragma once
#include <eosio/eosio.hpp>

using namespace eosio;
using namespace std;

namespace freedao {

  // system table
  struct[[ eosio::table("system"), eosio::contract("freeosgov") ]] system {
    time_point init;
    uint32_t usercount;
    uint32_t claimevents;
    uint32_t votes;
    asset cls;
    
    uint64_t primary_key() const {
      return 0;
    } // return a constant (0 in this case) to ensure a single-row table
  };
  using system_index = eosio::multi_index<"system"_n, system>;

  // parameters table
  struct[[ eosio::table("parameters"), eosio::contract("freeosgov") ]] parameter {
    name paramname;
    string value;

    uint64_t primary_key() const { return paramname.value; }
  };
  using parameters_index = eosio::multi_index<"parameters"_n, parameter>;

  // users table
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

  uint64_t primary_key() const { return 0; }
};
using users_index = eosio::multi_index<"users"_n, user>;

}