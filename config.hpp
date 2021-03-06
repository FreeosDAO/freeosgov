#pragma once
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include "freeosgov.hpp"
#include "constants.hpp"

using namespace eosio;
using namespace freedao;

// get string parameter value
string freeosgov::get_parameter(name paramname) {
  parameters_index parameters_table(get_self(), get_self().value);
  auto parameter_iterator = parameters_table.find(paramname.value);

  std::string assert_msg = paramname.to_string() + " is not defined in the parameters table";
  check(parameter_iterator != parameters_table.end(), assert_msg);

  return parameter_iterator->value;
}

// get double parameter value
double freeosgov::get_dparameter(name paramname) {
  dparameters_index dparameters_table(get_self(), get_self().value);
  auto dparameter_iterator = dparameters_table.find(paramname.value);

  std::string assert_msg = paramname.to_string() + " is not defined in the dparameters table";
  check(dparameter_iterator != dparameters_table.end(), assert_msg);

  return dparameter_iterator->value;
}

//void freeosgov::svrsplit(uint)

// ACTION
void freeosgov::paramupsert(name paramname, std::string value) {

  require_auth(get_self());
  parameters_index parameters_table(get_self(), get_self().value);
  auto parameter_iterator = parameters_table.find(paramname.value);

  // check if the parameter is in the table or not
  if (parameter_iterator == parameters_table.end()) {
    // the parameter is not in the table, so insert
    parameters_table.emplace(get_self(), [&](auto &parameter) {
      parameter.paramname = paramname;
      parameter.value = value;
    });

  } else {
    // the parameter is in the table, so update
    parameters_table.modify(parameter_iterator, get_self(), [&](auto &parameter) {
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

// ACTION
void freeosgov::dparamupsert(name paramname, double dvalue) {

  require_auth(get_self());
  dparameters_index dparameters_table(get_self(), get_self().value);
  auto dparameter_iterator = dparameters_table.find(paramname.value);

  // check if the parameter is in the table or not
  if (dparameter_iterator == dparameters_table.end()) {
    // the parameter is not in the table, so insert
    dparameters_table.emplace(get_self(), [&](auto &dparameter) {
      dparameter.paramname = paramname;
      dparameter.value = dvalue;
    });

  } else {
    // the parameter is in the table, so update
    dparameters_table.modify(dparameter_iterator, get_self(), [&](auto &dparameter) {
      dparameter.value = dvalue;
    });
  }
}

// erase dparameter from the table
// ACTION
void freeosgov::dparamerase(name paramname) {
  require_auth(_self);

  dparameters_index dparameters_table(get_self(), get_self().value);
  auto dparameter_iterator = dparameters_table.find(paramname.value);

  // check if the parameter is in the table or not
  check(dparameter_iterator != dparameters_table.end(),
        "double parameter does not exist");

  // the parameter is in the table, so delete
  dparameters_table.erase(dparameter_iterator);
}



// add an account to the transferers whitelist
// ACTION
void freeosgov::transfadd(name account) {
  require_auth(get_self());

  transferers_index transferers_table(get_self(), get_self().value);
  transferers_table.emplace(
      get_self(), [&](auto &transferer) { transferer.account = account; });
}

// ACTION
void freeosgov::transferase(name account) {
  require_auth(get_self());

  transferers_index transferers_table(get_self(), get_self().value);
  auto transferer_iterator = transferers_table.find(account.value);

  // check if the account is in the table
  check(transferer_iterator != transferers_table.end(),
        "account is not in the transferers table");

  // the account is in the table, so delete
  transferers_table.erase(transferer_iterator);
}

// add an account to the issuers whitelist
// ACTION
void freeosgov::minteradd(name account) {
  require_auth(get_self());

  minters_index minters_table(get_self(), get_self().value);
  minters_table.emplace(_self, [&](auto &issuer) { issuer.account = account; });
}

// erase an account from the issuers whitelist
// ACTION
void freeosgov::mintererase(name account) {
  require_auth(get_self());

  minters_index minters_table(get_self(), get_self().value);
  auto minter_iterator = minters_table.find(account.value);

  // check if the account is in the table
  check(minter_iterator != minters_table.end(),
        "account is not in the minters table");

  // the account is in the table, so delete
  minters_table.erase(minter_iterator);
}

// add an account to the burners whitelist
// ACTION
void freeosgov::burneradd(name account) {
  require_auth(get_self());

  burners_index burners_table(get_self(), get_self().value);
  burners_table.emplace(_self, [&](auto &burner) { burner.account = account; });
}

// erase an account from the burners whitelist
// ACTION
void freeosgov::burnererase(name account) {
  require_auth(get_self());

  burners_index burners_table(get_self(), get_self().value);
  auto burner_iterator = burners_table.find(account.value);

  // check if the account is in the table
  check(burner_iterator != burners_table.end(),
        "account is not in the burners table");

  // the account is in the table, so delete
  burners_table.erase(burner_iterator);
}

// ACTION
void freeosgov::currentrate(double price) {

  // check if the exchange account is calling this action, or the contract itself
  parameters_index parameters_table(get_self(), get_self().value);
  auto parameter_iterator = parameters_table.find(name("exchangeacc").value);
  if (parameter_iterator != parameters_table.end()) {
    require_auth(name(parameter_iterator->value));
  } else {
    require_auth(get_self());
  }

  check(price > 0.0, "current rate must be positive");

  exchange_index rates_table(get_self(), get_self().value);
  auto rate_iterator = rates_table.begin();

  // check if the rate exists in the table
  if (rate_iterator == rates_table.end()) {
    // the rate is not in the table, so insert
    rates_table.emplace(get_self(), [&](auto &rate) { rate.currentprice = price; });

  } else {
    // the rate is in the table, so update
    rates_table.modify(rate_iterator, _self,
                       [&](auto &rate) { rate.currentprice = price; });
  }
}

// ACTION
void freeosgov::targetrate(double exchangerate) {

  require_auth(get_self());

  check(exchangerate > 0.0, "target rate must be positive");

  double new_exchangerate = exchangerate;

  // ensure it is not set below the hardcoded floor
  if (new_exchangerate < HARD_EXCHANGE_RATE_FLOOR) {
    new_exchangerate = HARD_EXCHANGE_RATE_FLOOR;
  }

  exchange_index rates_table(get_self(), get_self().value);
  auto rate_iterator = rates_table.begin();

  // check if the rate exists in the table
  if (rate_iterator == rates_table.end()) {
    // the rate is not in the table, so insert
    rates_table.emplace(
        get_self(), [&](auto &rate) { rate.targetprice = new_exchangerate; });

  } else {
    // the rate is in the table, so update
    rates_table.modify(rate_iterator, get_self(), [&](auto &rate) {
      rate.targetprice = new_exchangerate;
    });
  }
}


