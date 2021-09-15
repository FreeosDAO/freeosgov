#include "freeosgov.hpp"
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include "tables.hpp"
#include "config.hpp"

namespace freedao {

using namespace eosio;

const std::string VERSION = "0.0.1";

// ACTION
void freeosgov::version() {
  std::string version_message = "Version = " + VERSION;

  check(false, version_message);
}

// ACTION
void freeosgov::init(time_point iterations_start) {

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

// functions
bool freeosgov::is_survey_period() {
  // default return is false
  bool result = true;

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

  if (iteration_secs >= surveystart && iteration_secs <= surveyend) result = true;

  return result;
}

} // end of namespace freedao