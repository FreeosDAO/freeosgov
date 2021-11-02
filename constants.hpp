#pragma once

using namespace std;

const string STAKE_CURRENCY_CODE = "FOOBAR";
const uint8_t STAKE_CURRENCY_PRECISION = 6;
const symbol STAKE_CURRENCY_SYMBOL = symbol(STAKE_CURRENCY_CODE, STAKE_CURRENCY_PRECISION);

const string POINT_CURRENCY_CODE = "POINT";
const uint8_t POINT_CURRENCY_PRECISION = 4;
const symbol POINT_CURRENCY_SYMBOL = symbol(POINT_CURRENCY_CODE, POINT_CURRENCY_PRECISION);

const name AIRCLAIM_CONTRACT = name("freeosd");     // TODO: ifdef for test/production
const name VERIFICATION_CONTRACT = name("eosio.proton"); // TODO: ifdef for test/production

const uint32_t ITERATION_LENGTH_SECONDS = 604800;   // 86400 = day, 604800 = week

const double HARD_EXCHANGE_RATE_FLOOR = 0.0167;
