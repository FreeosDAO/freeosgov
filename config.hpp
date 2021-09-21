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


// add an account to the transferers whitelist
// ACTION
void freeosgov::transfadd(name account) {
  require_auth(_self);

  transferers_index transferers_table(get_self(), get_self().value);
  transferers_table.emplace(
      _self, [&](auto &transferer) { transferer.account = account; });
}

// ACTION
void freeosgov::transferase(name account) {
  require_auth(_self);

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
  require_auth(_self);

  minters_index minters_table(get_self(), get_self().value);
  minters_table.emplace(_self, [&](auto &issuer) { issuer.account = account; });
}

// erase an account from the issuers whitelist
// ACTION
void freeosgov::mintererase(name account) {
  require_auth(_self);

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
  require_auth(_self);

  burners_index burners_table(get_self(), get_self().value);
  burners_table.emplace(_self, [&](auto &burner) { burner.account = account; });
}

// erase an account from the burners whitelist
// ACTION
void freeosgov::burnererase(name account) {
  require_auth(_self);

  burners_index burners_table(get_self(), get_self().value);
  auto burner_iterator = burners_table.find(account.value);

  // check if the account is in the table
  check(burner_iterator != burners_table.end(),
        "account is not in the burners table");

  // the account is in the table, so delete
  burners_table.erase(burner_iterator);
}

