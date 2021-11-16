#include "freeosgov.hpp"
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include "tables.hpp"
#include "config.hpp"
#include "identity.hpp"
#include "survey.hpp"
#include "vote.hpp"
#include "ratify.hpp"
#include "points.hpp"
#include "claim.hpp"

namespace freedao {

using namespace eosio;
using namespace std;

const std::string VERSION = "0.6.0";

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
    system_table.emplace(get_self(), [&](auto &sys) {
      sys.init = iterations_start;
      sys.cls = asset(0, POINT_CURRENCY_SYMBOL);
      });
  } else {
    // modify system record
    system_table.modify(system_iterator, _self, [&](auto &sys) { sys.init = iterations_start; });
  }

  // create the survey, vote and ratify records if they don't already exist
  // survey
  survey_init();

  // vote
  vote_init();

  // ratify
  ratify_init();

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

  if (action == "clear rewards") {
    rewards_index rewards_table(get_self(), get_self().value);
    auto rewards_iterator = rewards_table.begin();
    while (rewards_iterator != rewards_table.end()) {
      rewards_iterator = rewards_table.erase(rewards_iterator);
    }
  }

  if (action == "set cls") {
    system_index system_table(get_self(), get_self().value);
    auto system_iterator = system_table.begin();
    check(system_iterator != system_table.end(), "system record is undefined");

    system_table.modify(system_iterator, _self, [&](auto &sys) {
      sys.cls = asset(231000000000, POINT_CURRENCY_SYMBOL);
    });
  }

  if (action == "get user cls") {
    asset amount = asset(0, POINT_CURRENCY_SYMBOL);
    amount += UCLS; // add to the CLS for the verified user
    amount += PARTNER_CLS_ADDITION; // add to CLS for the partners
    check(false, amount.to_string());
  }

  if (action == "set usercount") {
    system_index system_table(get_self(), get_self().value);
    auto system_iterator = system_table.begin();
    check(system_iterator != system_table.end(), "system record is undefined");

    system_table.modify(system_iterator, _self, [&](auto &sys) {
      sys.usercount = 1;
    });
  }

  if (action == "system clear") {
    system_index system_table(get_self(), get_self().value);
    auto system_iterator = system_table.begin();
    system_table.erase(system_iterator);
  }

  if (action == "system restore") {
    system_index system_table(get_self(), get_self().value);
    system_table.emplace(
        get_self(), [&](auto &sys) {
          sys.usercount = 6;
          sys.cls = asset(231000000000, POINT_CURRENCY_SYMBOL);
          sys.claimevents = 0;
          sys.iteration = 9;
          // sys.particpants = 0;
          // sys.init = time_point("2021-09-15T00:00:00.000");
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


  if (action == "set svr") {
    svr_index svrs_table(get_self(), user.value);
    auto svr_iterator = svrs_table.begin();

    if (svr_iterator == svrs_table.end()) {
        // emplace
        svrs_table.emplace(get_self(), [&](auto &svr) { ; });
        svr_iterator = svrs_table.begin();
    } else {
        svrs_table.modify(svr_iterator, _self, [&](auto &svr) {
          svr.survey1 = 1;
          svr.survey2 = 1;
          svr.survey3 = 1;
          svr.survey4 = 1;
          svr.vote1 = 1;
          svr.vote2 = 1;
          svr.vote3 = 1;
          svr.vote4 = 1;
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

// ACTION
void freeosgov::tick() {

  // are we on a new iteration?
  system_index system_table(get_self(), get_self().value);
  auto system_iterator = system_table.begin();
  check(system_iterator != system_table.end(), "system record is undefined");

  uint16_t recorded_iteration = system_iterator->iteration;
  uint16_t actual_iteration = current_iteration();

  if (recorded_iteration != actual_iteration) {
    // update the recorded iteration
    system_table.modify(system_iterator, _self, [&](auto &sys) {
      sys.iteration = actual_iteration;
    });

    // run the new iteration service routine
    trigger_new_iteration(actual_iteration);
  }
}

void freeosgov::trigger_new_iteration(uint32_t new_iteration) {

  // if it is iteration 1 then nothing to, so return
  if (new_iteration == 1) return;

  // // record/update the old and new iterations - and take snapshot of the CLS at this point
  system_index system_table(get_self(), get_self().value);
  auto system_iterator = system_table.begin();
  check(system_iterator != system_table.end(), "system record is undefined");

  asset cls_snapshot = system_iterator->cls;

  uint32_t old_iteration = system_iterator->iteration;
  system_table.modify(system_iterator, _self, [&](auto &sys) {
    sys.iteration = new_iteration;
  });

  // capture the data we need from the system, vote and ratify records
  // 1. system record
  uint32_t participants = system_iterator->participants;

  // 2. vote record
  votescast_index vote_table(get_self(), get_self().value);
  auto vote_iterator = vote_table.begin();
  check(vote_iterator != vote_table.end(), "vote record is undefined");
  // issuance_rate
  double issuance_rate = vote_iterator->q1average;
  // mint fee percent
  double mint_fee_percent = vote_iterator->q2average;
  // locking threshold
  double locking_threshold = vote_iterator->q3average;
  // pool decision
  bool pool = vote_iterator->q4choice1 >= vote_iterator->q4choice2;
  // burn decision
  bool burn = !pool;

  // 3. ratify record
  ratify_index ratify_table(get_self(), get_self().value);
  auto ratify_iterator = ratify_table.begin();
  check(ratify_iterator != ratify_table.end(), "ratify record is undefined");
  // ratify decision
  bool ratified = ratify_iterator->ratified >= (ratify_iterator->participants / 2);


  // populate the rewards table - take a snapshot of the cls, vote results and ratify result
  rewards_index rewards_table(get_self(), get_self().value);
  rewards_table.emplace(_self, [&](auto &rwd) {
      rwd.iteration = old_iteration;
      rwd.participants = participants;
      rwd.iteration_cls = cls_snapshot;
      rwd.issuance_rate = issuance_rate;
      rwd.mint_fee_percent = mint_fee_percent;
      rwd.locking_threshold = locking_threshold;
      rwd.pool = pool;
      rwd.burn = burn;
      rwd.ratified = ratified;
    });

  // delete old rewards records, if necessary
  if (old_iteration > 4) {
    auto rewards_iterator = rewards_table.begin();
    while (rewards_iterator->iteration <= (old_iteration - 4)) {
      rewards_iterator = rewards_table.erase(rewards_iterator);
    }
  }
  

}

} // end of namespace freedao