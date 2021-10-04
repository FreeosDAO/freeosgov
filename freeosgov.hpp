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
  [[eosio::action]] void maintain(string action, name user);

  // identity actions
  [[eosio::action]] void reguser(name user);

  // config actions
  [[eosio::action]] void paramupsert(name paramname, std::string value);
  [[eosio::action]] void paramerase(name paramname);
  [[eosio::action]] void transfadd(name account);
  [[eosio::action]] void transferase(name account);
  [[eosio::action]] void minteradd(name account);
  [[eosio::action]] void mintererase(name account);
  [[eosio::action]] void burneradd(name account);
  [[eosio::action]] void burnererase(name account);

  // survey actions (In survey.hpp)
  [[eosio::action]] void survey( name user, bool r0,  bool r1,  bool r2,  // Question 1
                                uint8_t r3,                    // Question 2 - slider
                                bool r4,  bool r5,  bool r6,   // Question 3  
                                uint8_t r7,                    // Question 4 - slider
                                bool r8,  bool r9,  bool r10,  // Question 5
                                bool r11, bool r12, bool r13, 
                                bool r14, bool r15, bool r16); // Question 6 

  // vote actions
  [[eosio::action]] void voteflow(name user, string response);

  // ratify actions
  [[eosio::action]] void ratifyflow(name user, bool ratify);

  // claim actions
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
  void surveyinit(); // In survey.hpp

  // functions
  bool is_action_period(string action);
  uint16_t current_iteration();
  bool is_registered(name user);
};

} // end of namespace freedao