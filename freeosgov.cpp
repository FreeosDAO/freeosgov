#include "freeosgov.hpp"
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include "tables.hpp"
#include "config.hpp"
#include "identity.hpp"
#include "survey.hpp"
#include "voteflow.hpp"
#include "ratifyflow.hpp"
#include "points.hpp"

namespace freedao {

using namespace eosio;
using namespace std;

const std::string VERSION = "0.0.20";

// ACTION
void freeosgov::version() {
  string version_message = "Version = " + VERSION + ", Iteration = " + to_string(current_iteration());

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

      // decrement number of users
      system_index system_table(get_self(), get_self().value);
      auto system_iterator = system_table.begin();
      check(system_iterator != system_table.end(), "system record is undefined");

      system_table.modify(system_iterator, _self, [&](auto &sys) {
        sys.usercount = sys.usercount - 1;
      });
    }

  }

  if (action == "set usercount") {
    system_index system_table(get_self(), get_self().value);
      auto system_iterator = system_table.begin();
      check(system_iterator != system_table.end(), "system record is undefined");

      system_table.modify(system_iterator, _self, [&](auto &sys) {
        sys.usercount = 1;
      });
  }

  if (action == "is registered") {
    if (is_registered(user)) {
      check(false, "user is registered");
    } else {
      check(false, "user is not registered");
    }
  }

  if (action == "survey period") {
    if (is_action_period("survey") == true) {
      check(false, "In survey period");
    } else {
      check(false, "Outside of survey period");
    }
  }

  if (action == "vote period") {
    if (is_action_period("vote") == true) {
      check(false, "In vote period");
    } else {
      check(false, "Outside of vote period");
    }
  }
  
  if (action == "ratify period") {
    if (is_action_period("ratify") == true) {
      check(false, "In ratify period");
    } else {
      check(false, "Outside of ratify period");
    }
  }

  if (action == "clear svr") {
    svr_index svrs_table(get_self(), user.value);
    auto svr_iterator = svrs_table.begin();

    if (svr_iterator == svrs_table.end()) {
        // emplace
        svrs_table.emplace(get_self(), [&](auto &svr) { ; });
        svr_iterator = svrs_table.begin();
    } else {
        svrs_table.modify(svr_iterator, _self, [&](auto &svr) {
          svr.survey1 = 0;
          svr.survey2 = 0;
          svr.survey3 = 0;
          svr.survey4 = 0;
          svr.vote1 = 0;
          svr.vote2 = 0;
          svr.vote3 = 0;
          svr.vote4 = 0;
          svr.ratify1 = 0;
          svr.ratify2 = 0;
          svr.ratify3 = 0;
          svr.ratify4 = 0;
        });
    }
    
  }

}



// helper functions

// are we in the survey, vote or ratify period?
bool freeosgov::is_action_period(string action) {
  // default return is false
  bool result = false;

  string action_parameter_start = action + "start";
  string action_parameter_end = action + "end";
  string err_msg_start = action_parameter_start + " is undefined";
  string err_msg_end = action_parameter_end + " is undefined";

  // get the start of freeos system time
  system_index system_table(get_self(), get_self().value);
  auto system_iterator = system_table.begin();
  check(system_iterator != system_table.end(), "system record is undefined");
  time_point init = system_iterator->init;

  // how far are we into the current iteration?
  uint64_t now_secs = current_time_point().sec_since_epoch();
  uint64_t init_secs = init.sec_since_epoch();
  uint32_t iteration_secs = (now_secs - init_secs) % ITERATION_LENGTH_SECONDS;

  // get the config parameters for surveystart and surveyend
  parameters_index parameters_table(get_self(), get_self().value);

  auto parameter_iterator = parameters_table.find(name(action_parameter_start).value);
  check(parameter_iterator != parameters_table.end(), err_msg_start);
  uint32_t action_start = stoi(parameter_iterator->value);

  parameter_iterator = parameters_table.find(name(action_parameter_end).value);
  check(parameter_iterator != parameters_table.end(), err_msg_end);
  uint32_t action_end = stoi(parameter_iterator->value);
  
  if (iteration_secs >= action_start && iteration_secs <= action_end) {
    result = true;
  }

  /* string debug_msg = "now_secs=" + to_string(now_secs) + ", init_secs=" + to_string(init_secs) +
   ", iteration_secs=" + to_string(iteration_secs) + ", surveystart=" + to_string(surveystart) + ", surveyend=" + to_string(surveyend);
  check(false, debug_msg); */

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
    iteration = ((now_secs - init_secs) / ITERATION_LENGTH_SECONDS) + 1;
  }
  
  return iteration;
}

} // end of namespace freedao