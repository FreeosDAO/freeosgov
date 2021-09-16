#include "freeosgov.hpp"
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include "tables.hpp"
#include "config.hpp"
#include "identity.hpp"
#include "survey.hpp"

namespace freedao {

using namespace eosio;
using namespace std;

const std::string VERSION = "0.0.2";

// ACTION
void freeosgov::version() {
  std::string version_message = "Version = " + VERSION;

  check(false, version_message);
}

// ACTION
void freeosgov::init(time_point iterations_start) {

  require_auth(get_self());

  system_index system_table(get_self(), get_self().value);
  auto system_iterator = system_table.begin();
  if (system_iterator == system_table.end()) {
    // insert system record
    system_table.emplace(get_self(), [&](auto &sys) { sys.init = iterations_start; });
  } else {
    // modify system record
    system_table.modify(system_iterator, _self, [&](auto &sys) { sys.init = iterations_start; });
  }
}

// ACTION
// maintenance actions - TODO: delete from production
void freeosgov::maintain(string action, name user) {

  require_auth(get_self());

  if (action == "erase user") {
    users_index users_table(get_self(), user.value);
    auto user_iterator = users_table.begin();

    if (user_iterator != users_table.end()) {
      users_table.erase(user_iterator);
    }
  }

  if (action == "is registered") {
    if (is_registered(user)) {
      check(false, "user is registered");
    } else {
      check(false, "user is not registered");
    }
  }

}



// helper functions

// are we in the survey period?
bool freeosgov::is_survey_period() {
  // default return is false
  bool result = false;

  // get the start of freeos system time
  system_index system_table(get_self(), get_self().value);
  auto system_iterator = system_table.begin();
  check(system_iterator != system_table.end(), "system record is undefined");
  time_point init = system_iterator->init;

  // how far are we into the current iteration?
  uint64_t now_secs = current_time_point().time_since_epoch()._count;
  uint64_t init_secs = init.time_since_epoch()._count;
  uint32_t iteration_secs = (now_secs - init_secs) % 604800;

  // get the config parameters for surveystart and surveyend
  parameters_index parameters_table(get_self(), get_self().value);

  auto parameter_iterator = parameters_table.find(name("surveystart").value);
  check(parameter_iterator != parameters_table.end(), "surveystart is undefined");
  uint16_t surveystart = stoi(parameter_iterator->value);

  parameter_iterator = parameters_table.find(name("surveyend").value);
  check(parameter_iterator != parameters_table.end(), "surveyend is undefined");
  uint16_t surveyend = stoi(parameter_iterator->value);

  if (iteration_secs >= surveystart && iteration_secs <= surveyend) {
    result = true;
  }

  return result;
}

// is user registered?
bool freeosgov::is_registered(name user) {
  users_index users_table(get_self(), user.value);
  auto user_iterator = users_table.begin();

  if (user_iterator == users_table.end()) {
    return false;
  } else {
    return true;
  }
}

// which iteration are we in?
uint16_t freeosgov::current_iteration() {

  uint16_t iteration = 0;

  // get the start of freeos system time
  system_index system_table(get_self(), get_self().value);
  auto system_iterator = system_table.begin();
  check(system_iterator != system_table.end(), "system record is undefined");
  time_point init = system_iterator->init;

  // how far are we into the current iteration?
  uint64_t now_secs = current_time_point().sec_since_epoch();
  uint64_t init_secs = init.sec_since_epoch();

  if (now_secs >= init_secs) {
    iteration = ((now_secs - init_secs) / 604800) + 1;
  }
  
  return iteration;
}

} // end of namespace freedao