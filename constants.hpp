#pragma once

using namespace std;

#define _STRINGIZE(x) #x
#define STRINGIZE(x) _STRINGIZE(x)

std::string freeby_acct = STRINGIZE(FREEBY);

// Currency constants
const string STAKE_CURRENCY_CODE = "XPR";
const uint8_t STAKE_CURRENCY_PRECISION = 4;
const symbol STAKE_CURRENCY_SYMBOL = symbol(STAKE_CURRENCY_CODE, STAKE_CURRENCY_PRECISION);

const string POINT_CURRENCY_CODE = "POINT";
const uint8_t POINT_CURRENCY_PRECISION = 4;
const symbol POINT_CURRENCY_SYMBOL = symbol(POINT_CURRENCY_CODE, POINT_CURRENCY_PRECISION);

const string FREEBY_CURRENCY_CODE = "FREEBY";
const uint8_t FREEBY_CURRENCY_PRECISION = 4;
const symbol FREEBY_CURRENCY_SYMBOL = symbol(FREEBY_CURRENCY_CODE, FREEBY_CURRENCY_PRECISION);

const string FREEOS_CURRENCY_CODE = "FREEOS";
const uint8_t FREEOS_CURRENCY_PRECISION = 4;
const symbol FREEOS_CURRENCY_SYMBOL = symbol(FREEOS_CURRENCY_CODE, FREEOS_CURRENCY_PRECISION);


const name AIRCLAIM_CONTRACT = name("freeosd");     // TODO: ifdef for test/production
const name VERIFICATION_CONTRACT = name("eosio.proton"); // TODO: ifdef for test/production

const uint32_t ITERATION_LENGTH_SECONDS = 3600; // 604800;   // 86400 = day, 604800 = week

const double HARD_EXCHANGE_RATE_FLOOR = 0.0167;

// User contribution to Conditionally Limited Supply
const asset UCLS = asset(35000000000, POINT_CURRENCY_SYMBOL);

// Partner addition factor - ie. proportion of user CLS for partners
const asset PARTNER_CLS_ADDITION = asset(3500000000, POINT_CURRENCY_SYMBOL);

// Issuance percentage (expressed as floating point) of CLS
const double ISSUANCE_PROPORTION_OF_CLS = 0.000286;
