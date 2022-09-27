#pragma once
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include "freeosgov.hpp"
#include "constants.hpp"

using namespace eosio;
using namespace freedao;

/** @defgroup config Configuration
 *  These Actions and functions are related to configuring the dApp.
 *  @{
 */


/**
 * It returns the string value of a parameter from the parameters table
 * 
 * @param paramname The name of the parameter record to retrieve from the parameters table.
 * 
 * @return The string value of the parameter.
 */
string freeosgov::get_parameter(name paramname) {
  parameters_index parameters_table(get_self(), get_self().value);
  auto parameter_iterator = parameters_table.find(paramname.value);

  std::string assert_msg = paramname.to_string() + " is not defined in the parameters table";
  check(parameter_iterator != parameters_table.end(), assert_msg);

  return parameter_iterator->value;
}

/**
 * It gets the integer value of a parameter from the parameters table
 * 
 * @param paramname The name of the parameter record to get from the parameters table.
 * 
 * @return The integer value of the parameter.
 */
int freeosgov::get_iparameter(name paramname) {
  parameters_index parameters_table(get_self(), get_self().value);
  auto parameter_iterator = parameters_table.find(paramname.value);

  std::string assert_msg = paramname.to_string() + " is not defined in the parameters table";
  check(parameter_iterator != parameters_table.end(), assert_msg);

  return stoi(parameter_iterator->value);
}

/**
 * It gets the double value of a parameter from the dparameters (double parameters) table
 * 
 * @param paramname The name of the parameter record to get from the dparameters table.
 * 
 * @return The double value of the parameter.
 */
double freeosgov::get_dparameter(name paramname) {
  dparameters_index dparameters_table(get_self(), get_self().value);
  auto dparameter_iterator = dparameters_table.find(paramname.value);

  std::string assert_msg = paramname.to_string() + " is not defined in the dparameters table";
  check(dparameter_iterator != dparameters_table.end(), assert_msg);

  return dparameter_iterator->value;
}



/**
 * Action takes a parameter name and a value, and either inserts the parameter into the parameters table if it doesn't
 * exist, or updates the parameter if it does exist
 * 
 * @param paramname The name of the parameter.
 * @param value The string value of the parameter.
 */
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


/**
 * Action deletes a parameter from the parameters table
 * 
 * @param paramname The name of the parameter to be deleted.
 */
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


/**
 * Action either inserts a new double parameter into the table or updates an existing double parameter in
 * the table
 * 
 * @param paramname the name of the parameter in the dparameters table
 * @param the double value of the parameter
 */
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


/**
 * Action deletes a double parameter from the dparameters table
 * 
 * @param paramname The name of the parameter to be deleted.
 */
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


/**
 * Action adds an account to the transferers table: list of accounts that can transfer (allocate) POINT tokens
 * 
 * @param account The account that will be able to transfer POINT tokens.
 */
void freeosgov::transfadd(name account) {
  require_auth(get_self());

  transferers_index transferers_table(get_self(), get_self().value);
  transferers_table.emplace(
      get_self(), [&](auto &transferer) { transferer.account = account; });
}


/**
 * Action removes an account from the transferers table
 * 
 * @param account the account to be removed from the table
 */
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


/**
 * Action adds a new account to the minters table: list of accounts that can issue POINT tokens.
 * 
 * @param account The account that will be able to mint (issue) tokens.
 */
void freeosgov::minteradd(name account) {
  require_auth(get_self());

  minters_index minters_table(get_self(), get_self().value);
  minters_table.emplace(_self, [&](auto &issuer) { issuer.account = account; });
}


/**
 * Action removes an account from the minters table
 * 
 * @param account the account to be removed from the minters table
 */
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


/**
 * Action adds an account to the burners table: people who can burn (retire) POINT tokens
 * 
 * @param account The account that will be added to the burners table.
 */
void freeosgov::burneradd(name account) {
  require_auth(get_self());

  burners_index burners_table(get_self(), get_self().value);
  burners_table.emplace(_self, [&](auto &burner) { burner.account = account; });
}


/**
 * Action deletes an account from the burners table
 * 
 * @param account the account to be removed from the burners table
 */
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


/**
 * Action sets the current FREEOS usd price in the exchangerate table
 * 
 * @pre Must be run under the authority of the contract or the account defined by the 'exchangeacc' parameter
 * 
 * @param double price The current price of the token which must have a positive value
 */
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

/**
 * Action sets the target FREEOS usd price in the exchangerate table
 * 
 * @pre Must be run under the authority of the contract
 * 
 * @param double price The target price of the token which must have a positive value
 */
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


/**
 * Action takes a symbol and a contract name as arguments, and then either creates a new currency entry in
 * the currencies table or updates an existing one
 * 
 * @param symbol The symbol of the currency you want to add.
 * @param contract The contract that manages the token ledger.
 */
void freeosgov::currupsert(symbol symbol, name contract) {

  require_auth(get_self());

  currencies_index currencies_table(get_self(), get_self().value);
  auto curr_iterator = currencies_table.find(symbol.raw());

  if (curr_iterator == currencies_table.end()) {
    // emplace
    currencies_table.emplace(get_self(), [&](auto &curr) {
      curr.symbol = symbol;
      curr.contract = contract;
    });
  } else {
    // modify
    currencies_table.modify(curr_iterator, get_self(), [&](auto &curr) {
      curr.contract = contract;
    });
  }
}


/**
 * Action sets the USD rate for a given token currency
 * 
 * @pre Must be run under the authority of the contract or the account defined by the 'exchangeacc' parameter
 * 
 * @param symbol the symbol of the currency to set the rate for
 * @param usdrate The USD rate of the currency.
 */
void freeosgov::currsetrate(symbol symbol, double usdrate) {

  // check if the exchange account is calling this action, or the contract itself
  parameters_index parameters_table(get_self(), get_self().value);
  auto parameter_iterator = parameters_table.find(name("exchangeacc").value);
  if (parameter_iterator != parameters_table.end()) {
    require_auth(name(parameter_iterator->value));
  } else {
    require_auth(get_self());
  }

  currencies_index currencies_table(get_self(), get_self().value);
  auto curr_iterator = currencies_table.find(symbol.raw());

  check(curr_iterator != currencies_table.end(), "currency record not found");

  // modify
  currencies_table.modify(curr_iterator, get_self(), [&](auto &curr) {
    curr.usdrate = usdrate;
  });
}


/**
 * Action erases a token currency from the currencies table
 * 
 * @param symbol The symbol of the currency to be erased.
 */
void freeosgov::currerase(symbol symbol) {

  require_auth(get_self());

  currencies_index currencies_table(get_self(), get_self().value);
  auto curr_iterator = currencies_table.find(symbol.raw());

  check(curr_iterator != currencies_table.end(), "currency not found");

  currencies_table.erase(curr_iterator);
}

/** @} */ // end of config group
