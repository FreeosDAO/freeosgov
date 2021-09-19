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

  // survey actions
  [[eosio::action]] void surveyflow(name user, uint8_t q1response, uint8_t q2response, uint8_t q3response, uint8_t q4response, string q5response);

  // vote actions
  [[eosio::action]] void voteflow(name user, string response);

  // ratify actions
  [[eosio::action]] void ratifyflow(name user, bool ratify);

  // functions
  bool is_action_period(string action);
  uint16_t current_iteration();
  bool is_registered(name user);
  void initialise_survey();
};

} // end of namespace freedao