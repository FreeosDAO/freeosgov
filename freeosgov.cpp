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
#include "maintain.hpp"

namespace freedao {

using namespace eosio;
using namespace std;

const std::string VERSION = "0.10.0";

/** @defgroup core Core Functions
 *  These Actions and functions are related to core functionality.
 *  @{
 */

/**
 * Action prints out the version of the contract, the accounts that are used for the freeos tokens, freebi
 * tokens, and the freeos divide account, and the current iteration
 */
void freeosgov::version() {

  string freeosdiv_acct = get_parameter(name("freedaoacct"));
  string freebi_tokens_contract = get_parameter(name("freebitokens"));
  string freeos_tokens_contract = get_parameter(name("freeostokens"));

  std::string version_message = "version: " + VERSION + ", freeos tokens account: " + freeos_tokens_contract + ", freebi tokens account: " + freebi_tokens_contract +
                                + ", freeos divide account: " + freeosdiv_acct + ", iteration: " + std::to_string(current_iteration());

  check(false, version_message);
}


/**
 * Function checks if the master switch (parameter: masterswitch) is on (set to "1") or off.
 * The masterswitch parameter is checked by all user-initiated actions. In effect
 * this can be used to prohibit user activity in the event of a serious problem.
 * 
 * @return A boolean value.
 */
bool freeosgov::check_master_switch() {
  parameters_index parameters_table(get_self(), get_self().value);
  auto parameter_iterator = parameters_table.find(name("masterswitch").value);

  // check if the parameter is in the table or not
  if (parameter_iterator == parameters_table.end()) {
    // the parameter is not in the table, or table not found, return false
    // because it should be accessible (failsafe)
    return false;
  } else {
    // the parameter is in the table
    if (parameter_iterator->value.compare("1") == 0) {
      return true;
    } else {
      return false;
    }
  }
}


/**
 * Action initializes the system by creating the system record, the reward record for iteration 0, and the
 * survey, vote and ratify records if they don't already exist
 * 
 * @param iterations_start the time at which the first iteration will start
 * @param issuance_rate the amount of points to be issued per iteration
 * @param mint_fee_percent_freeos the percentage of the issuance that goes to the freeos.io team
 * @param mint_fee_percent_xpr the percentage of the issuance that goes to the XPR pool
 * @param mint_fee_percent_xusdc The percentage of the issuance that goes to the XUSDC pool.
 * @param locking_threshold the minimum percentage of a user's total points that must be locked in
 * order to participate in the vote
 * @param pool true if the rewards are to be distributed to the pool, false if they are to be burned
 */
void freeosgov::init(time_point iterations_start, double issuance_rate, double mint_fee_percent_freeos,
                    double mint_fee_percent_xpr, double mint_fee_percent_xusdc, double locking_threshold, bool pool) {

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
    system_table.modify(system_iterator, get_self(), [&](auto &sys) { sys.init = iterations_start; });
  }

  // create the reward record (for iteration 0) containing ratified (by system team) values for vote parameters
  rewards_index rewards_table(get_self(), get_self().value);
  rewards_table.emplace(get_self(), [&](auto &rwd) {
      rwd.iteration = 0;
      rwd.participants = 0;
      rwd.iteration_cls = asset(0, POINT_CURRENCY_SYMBOL);
      rwd.iteration_issuance = asset(0, POINT_CURRENCY_SYMBOL);
      rwd.participant_issuance = asset(0, POINT_CURRENCY_SYMBOL);
      rwd.issuance_rate = issuance_rate;
      rwd.mint_fee_percent = mint_fee_percent_freeos;
      rwd.mint_fee_percent_xpr = mint_fee_percent_xpr;
      rwd.mint_fee_percent_xusdc = mint_fee_percent_xusdc;
      rwd.locking_threshold = locking_threshold;
      rwd.pool = pool;
      rwd.burn = !pool;
      rwd.ratified = true;
    });

  // create the survey, vote and ratify records if they don't already exist
  // survey
  survey_init();

  // vote
  vote_init();

  // ratify
  ratify_init();

}


/**
 * If the current time is between the start and end of the action period, return true
 * 
 * @param action the name of the activity, i.e. "survey", "vote" or "ratify"
 * 
 * @return A boolean value indicating whether the activity period is current
 */
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

  // read the iteration length in seconds
  int iteration_length_secs = get_iparameter(name("iterationsec"));

  // how far are we into the current iteration?
  uint64_t now_secs = current_time_point().sec_since_epoch();
  uint64_t init_secs = init.sec_since_epoch();
  uint32_t iteration_secs = (now_secs - init_secs) % iteration_length_secs;

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

  return result;
}


/**
 * The function returns the current iteration number
 * 
 * @return The current iteration number.
 */
uint32_t freeosgov::current_iteration() {

  uint32_t iteration = 0;

  // get the start of freeos system time
  system_index system_table(get_self(), get_self().value);
  auto system_iterator = system_table.begin();
  check(system_iterator != system_table.end(), "system record is undefined");
  time_point init = system_iterator->init;

  // how far are we into the current iteration?
  uint64_t now_secs = current_time_point().sec_since_epoch();
  uint64_t init_secs = init.sec_since_epoch();

  // read the iteration length in seconds
  int iteration_secs = get_iparameter(name("iterationsec"));

  if (now_secs >= init_secs) {
    iteration = ((now_secs - init_secs) / iteration_secs) + 1;
  }
  
  return iteration;
}


/**
 * The `tick()` function is called every time a user runs an action. It checks that the system is
 * operational (i.e. that the `masterswitch` parameter is set to `1`). If the iteration has changed, the
 * trigger_new_iteration() function is called to perform app housekeeping operations
 */
void freeosgov::tick() {

  // check that system is operational (masterswitch parameter set to "1")
  check(check_master_switch(), MSG_FREEOS_SYSTEM_NOT_AVAILABLE);

  // are we on a new iteration?
  system_index system_table(get_self(), get_self().value);
  auto system_iterator = system_table.begin();
  check(system_iterator != system_table.end(), "system record is undefined");

  uint32_t recorded_iteration = system_iterator->iteration;
  uint32_t this_iteration = current_iteration();


  // set the new iteration in the system record
  if (this_iteration != recorded_iteration) {
    // run the new iteration change service routine
    trigger_new_iteration(this_iteration);

    // update the recorded iteration
    system_index system_table(get_self(), get_self().value);  // do a refresh of the system record as the previous system_iterator has old data 
    system_iterator = system_table.begin();                   // refresh the system iterator to get the record amended by modifications
    system_table.modify(system_iterator, get_self(), [&](auto &sys) {
      sys.iteration = this_iteration;
      sys.participants = 0;
    });

  }
}


/**
 * Function returns the double proportion of the total supply that should be locked, based on the current and target
 * exchange rates in the exchangerate table. For example, a target rate of $2 and a current rate of $1.5 returns a
 * lock proportion = 0.25. The proportion is capped at 0.9
 * 
 * @return The proportion of the amount of tokens that will be locked.
 */
double freeosgov::get_locked_proportion() {
  // default rate if exchange rate record not found, or if current price >= target price (so no need to lock)
  double proportion = 0.0;

  exchange_index exchangerate_table(get_self(), get_self().value);
  auto exchangerate_iterator = exchangerate_table.begin();

  // if the exchange rate exists in the table
  if (exchangerate_iterator != exchangerate_table.end()) {
    // get current and target rates
    double currentprice = exchangerate_iterator->currentprice;
    double targetprice = exchangerate_iterator->targetprice;

    if (targetprice > 0 && currentprice < targetprice) {
      proportion = 1.0 - (currentprice / targetprice);
    }
  } else {
    // use the default proportion specified in the 'lockpercent' parameter
    parameters_index parameters_table(get_self(), get_self().value);
    auto parameter_iterator = parameters_table.find(name("lockpercent").value);

    if (parameter_iterator != parameters_table.end()) {
      uint8_t int_percent = stoi(parameter_iterator->value);
      proportion = ((double)int_percent) / 100.0;
    }
  }

  // apply a cap of 0.9
  if (proportion > 0.9) {
    proportion = 0.9;
  }

  return proportion;
}


/**
 * If the exchange rate is favourable, i.e current rate > target rate (good times), increase the unlock percentage.
 * If the exchange rate is unfavourable (bad times), decrease the unlock percentage.
 * 
 * The rate of unlock percentage increases in good times according to a Fibonacci sequence, i.e. 1%, 2%, 3%, 5%, etc
 * The unlock percentage is capped at 21%.
 * 
 * Going into 'bad times' resets the unlock percentage to 0.
 * 
 * "failsafe" unlocking applies in bad times. Every so many weeks, users can unlock 15%. The interval is determined by
 * parameter: failsafefreq
 */
void freeosgov::update_unlock_percentage() {
  uint32_t current_unlock_percentage = 0;
  uint32_t new_unlock_percentage = 0;

  // get the system record
  system_index system_table(get_self(), get_self().value);
  auto system_iterator = system_table.begin();
  check(system_iterator != system_table.end(), "statistics record is not found");

  // find the current locked proportion. If 0.0f it means that the exchange rate is favourable
  float locked_proportion = get_locked_proportion();

  // Decide whether we are above target or below target price
  if (locked_proportion == 0.0f) {

    // favourable exchange rate, so implement the 'good times' strategy -
    // calculate the new unlock_percentage
    current_unlock_percentage = system_iterator->unlockpercent;

    // move the unlock_percentage on to next level as we have reached a 'good times' iteration
    switch (current_unlock_percentage) {
    case 0:
    case 15:
      new_unlock_percentage = 1;
      break;
    case 1:
      new_unlock_percentage = 2;
      break;
    case 2:
      new_unlock_percentage = 3;
      break;
    case 3:
      new_unlock_percentage = 5;
      break;
    case 5:
      new_unlock_percentage = 8;
      break;
    case 8:
      new_unlock_percentage = 13;
      break;
    case 13:
      new_unlock_percentage = 21;
      break;
    case 21:
      new_unlock_percentage = 21;
      break;
    default :
      new_unlock_percentage = 0;
      break;
    }

    // modify the system table with the new percentage. Also ensure the failsafe counter is set to 0.
    system_table.modify(system_iterator, get_self(), [&](auto &sys) {
      sys.unlockpercent = new_unlock_percentage;
      sys.unlockpercentiteration = current_iteration();
      sys.failsafecounter = 0;
    });

  } else {
    // unfavourable exchange rate, so implement the 'bad times' strategy
    // calculate failsafe unlock percentage - every Xth week of unfavourable
    // rate, set unlock percentage to 15%

    // get the unlock failsafe frequency - default is 24
    uint8_t failsafe_frequency = 24;

    // read the frequency from the freeosconfig 'parameters' table
    parameters_index parameters_table(get_self(), get_self().value);
    auto parameter_iterator = parameters_table.find(name("failsafefreq").value);

    if (parameter_iterator != parameters_table.end()) {
      failsafe_frequency = stoi(parameter_iterator->value);
    }

    // increment the failsafe_counter
    uint32_t failsafe_counter = system_iterator->failsafecounter;
    failsafe_counter++;

    // store the new failsafecounter and unlockpercent
    system_table.modify(system_iterator, get_self(), [&](auto &sys) {
      sys.failsafecounter = failsafe_counter % failsafe_frequency;
      sys.unlockpercent = (failsafe_counter == failsafe_frequency ? 15 : 0);
      sys.unlockpercentiteration = current_iteration();
    });
  }
}


/**
 * Function performs various housekeeping activities at the switchover to a new iteration.
 * It updates the system record with the new iteration, updates the locking threshold, calculates the
 * issuance for the iteration, updates the rewards record, updates the exchange rate record, deletes
 * old rewards records, and resets the survey, vote and ratify records
 * 
 * @param new_iteration the iteration number of the new iteration
 */
void freeosgov::trigger_new_iteration(uint32_t new_iteration) {
  double issuance_rate;
  double mint_fee_percent;
  double mint_fee_percent_xpr;
  double mint_fee_percent_xusdc;
  double locking_threshold;
  bool pool;
  bool burn;

  // if the transition is from iteration 0 to 1 there is nothing to do, so return
  if (new_iteration == 1) return;

  // update the locking parameters
  update_unlock_percentage();

  // record/update the old and new iterations - and take snapshot of the CLS at this point
  system_index system_table(get_self(), get_self().value);
  auto system_iterator = system_table.begin();
  check(system_iterator != system_table.end(), "system record is undefined");

  // get the previous reward record as we may have to propagate the existing values for vote parameters if the vote was not ratified
  rewards_index rewards_table(get_self(), get_self().value);
  auto last_reward_iterator = rewards_table.rbegin();
  check(last_reward_iterator != rewards_table.rend(), "previous reward table not found");

  // capture the data we need from the system, vote and ratify records
  // 1. system record
  uint32_t participants = system_iterator->participants;
  asset cls_snapshot = system_iterator->cls;
  uint32_t old_iteration = system_iterator->iteration;

  // 2. ratify record
  ratify_index ratify_table(get_self(), get_self().value);
  auto ratify_iterator = ratify_table.begin();
  check(ratify_iterator != ratify_table.end(), "ratify record is undefined");
  // ratify decision
  bool ratified = (ratify_iterator->ratified > 0 && (ratify_iterator->ratified >= ((ratify_iterator->participants + 1) / 2)));


  // 3. economic parameters
  if (ratified == true) {
    // if vote ratified, get the economic parameters from the vote record
    vote_index vote_table(get_self(), get_self().value);
    auto vote_iterator = vote_table.begin();
    check(vote_iterator != vote_table.end(), "vote record is undefined");
    // issuance_rate
    issuance_rate = vote_iterator->q1average / 100.0;
    // mint fee percent
    mint_fee_percent = vote_iterator->q2average;
    mint_fee_percent_xpr = vote_iterator->q2average_xpr;
    mint_fee_percent_xusdc = vote_iterator->q2average_xusdc;
    // locking threshold
    locking_threshold = vote_iterator->q3average;
    // pool decision
    pool = vote_iterator->q4choice1 >= vote_iterator->q4choice2;
    // burn decision
    burn = !pool;
  } else {
    // if vote not ratified, propagate existing values for issuance_rate, mint_fee_percent, locking_threshold, pool, burn in the event of non-ratification
    issuance_rate = last_reward_iterator->issuance_rate;
    mint_fee_percent = last_reward_iterator->mint_fee_percent;
    mint_fee_percent_xpr = last_reward_iterator->mint_fee_percent_xpr;
    mint_fee_percent_xusdc = last_reward_iterator->mint_fee_percent_xusdc;
    locking_threshold = last_reward_iterator->locking_threshold;
    pool = last_reward_iterator->pool;
    burn = last_reward_iterator->burn;
  }

  // 4. Calculate the total issuance (shared between participants) and the per-participant issuance for the iteration
  double ISSUANCE_PROPORTION_OF_CLS = get_dparameter(name("issuepropcls"));

  asset iteration_issuance = asset(0, POINT_CURRENCY_SYMBOL);
  asset participant_issuance = asset(0, POINT_CURRENCY_SYMBOL);
  if (ratified == true && participants > 0) {
    uint64_t iteration_issuance_amount = cls_snapshot.amount * ISSUANCE_PROPORTION_OF_CLS * issuance_rate;
    uint64_t participant_issuance_amount = iteration_issuance_amount / participants;

    iteration_issuance = asset(iteration_issuance_amount, POINT_CURRENCY_SYMBOL);
    participant_issuance = asset(participant_issuance_amount, POINT_CURRENCY_SYMBOL);
  }
  

  // populate the rewards table - take a snapshot of the cls, vote results and ratify result
  rewards_table.emplace(get_self(), [&](auto &rwd) {
      rwd.iteration = old_iteration;
      rwd.participants = participants;
      rwd.iteration_cls = cls_snapshot;
      rwd.iteration_issuance = iteration_issuance;
      rwd.participant_issuance = participant_issuance;
      rwd.issuance_rate = issuance_rate;
      rwd.mint_fee_percent = mint_fee_percent;
      rwd.mint_fee_percent_xpr = mint_fee_percent_xpr;
      rwd.mint_fee_percent_xusdc = mint_fee_percent_xusdc;
      rwd.locking_threshold = locking_threshold;
      rwd.pool = pool;
      rwd.burn = burn;
      rwd.ratified = ratified;
    });

    // write the locking threshold into the exchangerate table
    exchange_index exchangerate_table(get_self(), get_self().value);
    auto exchange_iterator = exchangerate_table.begin();
    check(exchange_iterator != exchangerate_table.end(), "exchangerate record not defined");
    exchangerate_table.modify(exchange_iterator, get_self(), [&](auto &exc) {
      exc.targetprice = locking_threshold;
    });

    
  // delete old rewards records, if necessary
  if (old_iteration > 4) {
    auto rewards_iterator = rewards_table.begin();
    while (rewards_iterator->iteration <= (old_iteration - 4)) {
      rewards_iterator = rewards_table.erase(rewards_iterator);
    }
  }
  
  // if the vote was ratified, subtract the iteration_issuance from the CLS
  if (ratified == true) {
    system_table.modify(system_iterator, get_self(), [&](auto &sys) {
      sys.cls -= iteration_issuance;
    });
  }

  // reset the survey, vote and ratify records, ready for the new iteration
  survey_reset();
  vote_reset();
  ratify_reset();

}

} // end of namespace freedao

/** @} */ // end of core group