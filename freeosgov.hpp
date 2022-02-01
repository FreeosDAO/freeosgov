#pragma once
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>

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
  [[eosio::action]] void tick();
  void trigger_new_iteration(uint32_t new_iterationß);

  // maintain actions TODO: remove in production version
  [[eosio::action]] void maintain(string action, name user);

  // identity actions
  [[eosio::action]] void reguser(name user);
  [[eosio::action]] void reregister(name user);
  bool is_user_verified(name user);

  // config actions
  [[eosio::action]] void paramupsert(name paramname, std::string value);
  [[eosio::action]] void paramerase(name paramname);
  [[eosio::action]] void dparamupsert(name paramname, double value);
  [[eosio::action]] void dparamerase(name paramname);
  [[eosio::action]] void transfadd(name account);
  [[eosio::action]] void transferase(name account);
  [[eosio::action]] void minteradd(name account);
  [[eosio::action]] void mintererase(name account);
  [[eosio::action]] void burneradd(name account);
  [[eosio::action]] void burnererase(name account);
  [[eosio::action]] void currentrate(double price);
  [[eosio::action]] void targetrate(double price);

  // survey actions (In survey.hpp)
  [[eosio::action]] void survey(name user, uint8_t q1response, uint8_t q2response, uint8_t q3response, uint8_t q4response, uint8_t q5choice1, uint8_t q5choice2, uint8_t q5choice3);
  void survey_init();
  void survey_reset();

  // vote actions/functions
  [[eosio::action]] void vote(name user, uint8_t q1response, uint8_t q2response, double q3response, string q4response, uint8_t q5response, uint8_t q6choice1, uint8_t q6choice2, uint8_t q6choice3);
  void vote_init();
  void vote_reset();

  // ratify actions/functions
  [[eosio::action]] void ratify(name user, bool ratify_vote);
  void ratify_init();
  void ratify_reset();

  // claim actions/functions
  [[eosio::action]] void claim(name user);

  // points actions and functions
  [[eosio::action]] void create(const name &issuer, const asset &maximum_supply);
  void issue(const name &to, const asset &quantity, const string &memo);
  void retire(const asset &quantity, const string &memo);
  [[eosio::action]] void allocate(const name &from, const name &to, const asset &quantity, const string &memo);
  [[eosio::action]] void mint(const name &minter, const name &to, const asset &quantity, const string &memo);
  [[eosio::action]] void burn(const name &burner, const asset &quantity, const string &memo);
  void transfer(const name &from, const name &to, const asset &quantity, const string &memo);
  void sub_balance(const name &owner, const asset &value);
  void add_balance(const name &owner, const asset &value, const name &ram_payer);
  [[eosio::action]] void mintfreeby(const name &owner, const asset &quantity);
  [[eosio::action]] void mintfreeos(const name &owner, const asset &quantity);


  // functions
  bool is_action_period(string action);
  uint16_t current_iteration();
  bool is_registered(name user);
  uint32_t user_last_active_iteration(name user);
  bool is_user_alive(name user);
  string get_parameter(name parameter);
  asset calculate_user_cls_addition();
};

} // end of namespace freedao