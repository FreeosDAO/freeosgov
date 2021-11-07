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
  [[eosio::action]] void currentrate(double price);
  [[eosio::action]] void targetrate(double price);

  // survey actions (In survey.hpp)
  [[eosio::action]] void survey( name user, bool r0,  bool r1,  bool r2, // Question 1
                                uint8_t r3,                     // Question 2 - slider
                                bool r4,  bool r5,  bool r6,    // Question 3  
                                uint8_t r7,                     // Question 4 - slider
                                uint8_t r8,  uint8_t r9,  uint8_t r10,  // Question 5
                                uint8_t r11, uint8_t r12, uint8_t r13);

  // vote actions
  [[eosio::action]] void vote(name user, uint8_t q1response, uint8_t q2response, double q3response, string q4response, uint8_t q5response, uint8_t q6choice1, uint8_t q6choice2, uint8_t q6choice3);
  [[eosio::action]] void testranges();

  // ratify actions
  [[eosio::action]] void ratify(name user, bool ratify_vote);

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
  [[eosio::action]] void mintfreeby(const name &owner, const asset &quantity);


  void surveyinit(); // In survey.hpp
  void initialise_vote();
  void initialise_ratify();

  // functions
  bool is_action_period(string action);
  uint16_t current_iteration();
  bool is_registered(name user);
  bool is_user_active(name user);
  string get_parameter(name parameter);
};

} // end of namespace freedao