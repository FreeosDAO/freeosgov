#pragma once

using namespace std;

/* old method for defining contract names, now replaced with parameters
#define _STRINGIZE(x) #x
#define STRINGIZE(x) _STRINGIZE(x)
std::string freebi_acct = STRINGIZE(FREEBY);
std::string freeos_acct = STRINGIZE(FREEOS);
*/

// Currency constants
const string XPR_CURRENCY_CODE = "FOOBAR";
const uint8_t XPR_CURRENCY_PRECISION = 6;
const uint32_t XPR_UNIT_MULTIPLIER = 1000000;
const symbol XPR_CURRENCY_SYMBOL = symbol(XPR_CURRENCY_CODE, XPR_CURRENCY_PRECISION);

const string XUSDC_CURRENCY_CODE = "ZEPI";
const uint8_t XUSDC_CURRENCY_PRECISION = 4;
const uint32_t XUSDC_UNIT_MULTIPLIER = 10000;
const symbol XUSDC_CURRENCY_SYMBOL = symbol(XUSDC_CURRENCY_CODE, XUSDC_CURRENCY_PRECISION);

const string POINT_CURRENCY_CODE = "POINT";
const uint8_t POINT_CURRENCY_PRECISION = 4;
const uint32_t POINT_UNIT_MULTIPLIER = 10000;
const symbol POINT_CURRENCY_SYMBOL = symbol(POINT_CURRENCY_CODE, POINT_CURRENCY_PRECISION);

const string AIRKEY_CURRENCY_CODE = "AIRKEY";
const uint8_t AIRKEY_CURRENCY_PRECISION = 0;
const uint32_t AIRKEY_UNIT_MULTIPLIER = 1;
const symbol AIRKEY_CURRENCY_SYMBOL = symbol(AIRKEY_CURRENCY_CODE, AIRKEY_CURRENCY_PRECISION);

const string FREEBI_CURRENCY_CODE = "BEEBI";
const uint8_t FREEBI_CURRENCY_PRECISION = 4;
const uint32_t FREEBI_UNIT_MULTIPLIER = 10000;
const symbol FREEBI_CURRENCY_SYMBOL = symbol(FREEBI_CURRENCY_CODE, FREEBI_CURRENCY_PRECISION);

const string FREEOS_CURRENCY_CODE = "BETA";
const uint8_t FREEOS_CURRENCY_PRECISION = 4;
const uint32_t FREEOS_UNIT_MULTIPLIER = 10000;
const symbol FREEOS_CURRENCY_SYMBOL = symbol(FREEOS_CURRENCY_CODE, FREEOS_CURRENCY_PRECISION);

const string MSG_FREEOS_SYSTEM_NOT_AVAILABLE = "Freeos system is not currently operating. Please try later";

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
