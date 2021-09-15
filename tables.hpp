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

  // miscellaneous parameters table
  struct[[ eosio::table("parameters"), eosio::contract("freeosgov") ]] parameter {
    name paramname;
    std::string value;

    uint64_t primary_key() const { return paramname.value; }
  };
  using parameters_index = eosio::multi_index<"parameters"_n, parameter>;

}