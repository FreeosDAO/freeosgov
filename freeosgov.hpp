#pragma once
//#include "identity.hpp"
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
//#include "config.hpp"

namespace freedao {
using namespace eosio;
using namespace std;

class[[eosio::contract("freeosgov")]] freeosgov : public contract {

public:
  using contract::contract;

  /**
   * version action.
   *
   * @details Prints the version of this contract.
   */
  [[eosio::action]] void version();
  [[eosio::action]] void init(time_point iterations_start);
  [[eosio::action]] void maintain(string action, name user);

  // identity actions
  [[eosio::action]] void reguser(name user);

  // config actions
  [[eosio::action]] void paramupsert(name paramname, std::string value);
  [[eosio::action]] void paramerase(name paramname);

  // functions
  bool is_survey_period();
  uint16_t current_iteration();
  bool is_registered(name user);
};

} // end of namespace freedao