#pragma once
#include "freeosgov.hpp"
#include <eosio/system.hpp>

//namespace freedao {

using namespace eosio;

// ACTION
void freeosgov::reguser(name user) {
  std::string version_message = "version = identity 0.2";

  check(false, version_message);
}

// the registered user table
struct[[ eosio::table("users"), eosio::contract("freeosgov") ]] user {
  asset stake;                    // how many XPR tokens staked
  char account_type;              // user's verification level
  uint32_t registered_iteration;  // when the user was registered
  uint32_t staked_iteration;      // the iteration in which the user staked their tokens
  uint32_t votes;                 // how many votes the user has made
  uint32_t issuances;             // total number of times the user has been issued with OPTIONs
  uint32_t last_issuance;         // the last iteration in which the user was issued with OPTIONs

  uint64_t primary_key() const { return stake.symbol.code().raw(); }
};
using users_index = eosio::multi_index<"users"_n, user>;