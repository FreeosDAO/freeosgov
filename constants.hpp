#pragma once

using namespace std;

#define _STRINGIZE(x) #x
#define STRINGIZE(x) _STRINGIZE(x)

std::string freeby_acct = STRINGIZE(FREEBY);
std::string freeos_acct = STRINGIZE(FREEOS);

// Currency constants
const string STAKE_CURRENCY_CODE = "XPR";
const uint8_t STAKE_CURRENCY_PRECISION = 4;
const symbol STAKE_CURRENCY_SYMBOL = symbol(STAKE_CURRENCY_CODE, STAKE_CURRENCY_PRECISION);

const string POINT_CURRENCY_CODE = "POINT";
const uint8_t POINT_CURRENCY_PRECISION = 4;
const symbol POINT_CURRENCY_SYMBOL = symbol(POINT_CURRENCY_CODE, POINT_CURRENCY_PRECISION);

const string AIRKEY_CURRENCY_CODE = "AIRKEY";
const uint8_t AIRKEY_CURRENCY_PRECISION = 0;
const symbol AIRKEY_CURRENCY_SYMBOL = symbol(AIRKEY_CURRENCY_CODE, AIRKEY_CURRENCY_PRECISION);

const string FREEBY_CURRENCY_CODE = "FREEBY";
const uint8_t FREEBY_CURRENCY_PRECISION = 4;
const symbol FREEBY_CURRENCY_SYMBOL = symbol(FREEBY_CURRENCY_CODE, FREEBY_CURRENCY_PRECISION);

const string FREEOS_CURRENCY_CODE = "FREEOS";
const uint8_t FREEOS_CURRENCY_PRECISION = 4;
const symbol FREEOS_CURRENCY_SYMBOL = symbol(FREEOS_CURRENCY_CODE, FREEOS_CURRENCY_PRECISION);


const name AIRCLAIM_CONTRACT = name("freeos5");     // TODO: we should not need for production as we are overwriting contract

#ifdef PRODUCTION
const name VERIFICATION_CONTRACT = name("eosio.proton");
#else
const name VERIFICATION_CONTRACT = name("freeosconfig");
#endif

const uint32_t ITERATION_LENGTH_SECONDS = 3600; // 604800;   // 86400 = day, 604800 = week

const double HARD_EXCHANGE_RATE_FLOOR = 0.0167;

// user CLS amount hard floor (in the absence of uclsamount parameter)
const int64_t UCLSAMOUNT = 3500000;

// Partner share hard floor (in the absence of the partnershare parameter)
const double PARTNERSHARE = 0.03;

// DAO share hard floor (in the absence of the daoshare parameter)
const double DAOSHARE = 0.07;

// Issuance percentage (expressed as floating point) of CLS
// const double ISSUANCE_PROPORTION_OF_CLS = 0.000286;  // now implemented as parameter 'issuepropcls'

// mint-fee-free allowance for AIRKEY holders
const int AIRKEY_MINT_FEE_FREE_ALLOWANCE = 5000;
