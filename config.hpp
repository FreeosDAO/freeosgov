//#pragma once
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include "freeosgov.hpp"

using namespace eosio;
using namespace freedao;



// ACTION
void freeosgov::paramupsert(name paramname, std::string value) {

  require_auth(_self);
  parameters_index parameters_table(get_self(), get_self().value);
  auto parameter_iterator = parameters_table.find(paramname.value);

  // check if the parameter is in the table or not
  if (parameter_iterator == parameters_table.end()) {
    // the parameter is not in the table, so insert
    parameters_table.emplace(_self, [&](auto &parameter) {
      parameter.paramname = paramname;
      parameter.value = value;
    });

  } else {
    // the parameter is in the table, so update
    parameters_table.modify(parameter_iterator, _self, [&](auto &parameter) {
      parameter.value = value;
    });
  }
}

// erase parameter from the table
// ACTION
void freeosgov::paramerase(name paramname) {
  require_auth(_self);

  parameters_index parameters_table(get_self(), get_self().value);
  auto parameter_iterator = parameters_table.find(paramname.value);

  // check if the parameter is in the table or not
  check(parameter_iterator != parameters_table.end(),
        "config parameter does not exist");

  // the parameter is in the table, so delete
  parameters_table.erase(parameter_iterator);
}


