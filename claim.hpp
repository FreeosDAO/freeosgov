//#pragma once
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include "freeosgov.hpp"
#include "tables.hpp"
#include "constants.hpp"

using namespace eosio;
using namespace freedao;
using namespace std;


// ACTION
void freeosgov::claim(name user) {

    const asset PAYMENT_SURVEY = asset(10000, POINT_CURRENCY_SYMBOL);   // 1.0000 POINT
    const asset PAYMENT_VOTE = asset(20000, POINT_CURRENCY_SYMBOL);     // 2.0000 POINT
    const asset PAYMENT_RATIFY = asset(30000, POINT_CURRENCY_SYMBOL);   // 3.0000 POINT

    require_auth(user);

    tick();

    uint32_t this_iteration = current_iteration();
    
    // is the system operational?
    check(this_iteration != 0, "The freeos system is not available at this time");

    // give the user 1 point for each survey, 2 for each vote, 3 points for ratify. TODO: needs algorithm for calculating claim amount

    // find the user's last issuance
    users_index users_table(get_self(), user.value);
    auto user_iterator = users_table.begin();
    check(user_iterator != users_table.end(), "user registration record is undefined");

    // find the user's svr record
    svr_index svr_table(get_self(), user.value);
    auto svr_iterator = svr_table.begin();
    check(svr_iterator != svr_table.end(), "user has not completed any votes or surveys");

    // get the last issuance
    uint32_t last_issuance = user_iterator->last_issuance;

    // check if user has already claimed in this iteration
    check(this_iteration != last_issuance, "you cannot claim more than once in an iteration");

    // check that we are not in iteration 1 - payments can only be made retrospectively
    check(this_iteration > 1, "claims are for previous iterations only");

    // determine which range of iterations we are paying for
    uint32_t earliest_payment_iteration = this_iteration > 4 ? this_iteration - 4 : 1;
    uint32_t latest_payment_iteration = this_iteration - 1;
    
    // initialise a running total of points to pay
    asset user_payment = asset(0, POINT_CURRENCY_SYMBOL);


    // get the previous 4 rewards records
    // TODO

    // payment for surveys
    if (svr_iterator->survey0 > last_issuance &&
        svr_iterator->survey0 >= earliest_payment_iteration &&
        svr_iterator->survey0 <= latest_payment_iteration) {
        user_payment += PAYMENT_SURVEY;
    }

    if (svr_iterator->survey1 > last_issuance &&
        svr_iterator->survey1 >= earliest_payment_iteration &&
        svr_iterator->survey1 <= latest_payment_iteration) {
        user_payment += PAYMENT_SURVEY;
    }

    if (svr_iterator->survey2 > last_issuance &&
        svr_iterator->survey2 >= earliest_payment_iteration &&
        svr_iterator->survey2 <= latest_payment_iteration) {
        user_payment += PAYMENT_SURVEY;
    }

    if (svr_iterator->survey3 > last_issuance &&
        svr_iterator->survey3 >= earliest_payment_iteration &&
        svr_iterator->survey3 <= latest_payment_iteration) {
        user_payment += PAYMENT_SURVEY;
    }

    if (svr_iterator->survey4 > last_issuance &&
        svr_iterator->survey4 >= earliest_payment_iteration &&
        svr_iterator->survey4 <= latest_payment_iteration) {
        user_payment += PAYMENT_SURVEY;
    }

    // payments for votes
    if (svr_iterator->vote0 > last_issuance &&
        svr_iterator->vote0 >= earliest_payment_iteration &&
        svr_iterator->vote0 <= latest_payment_iteration) {
        user_payment += PAYMENT_VOTE;
    }

    if (svr_iterator->vote1 > last_issuance &&
        svr_iterator->vote1 >= earliest_payment_iteration &&
        svr_iterator->vote1 <= latest_payment_iteration) {
        user_payment += PAYMENT_VOTE;
    }

    if (svr_iterator->vote2 > last_issuance &&
        svr_iterator->vote2 >= earliest_payment_iteration &&
        svr_iterator->vote2 <= latest_payment_iteration) {
        user_payment += PAYMENT_VOTE;
    }

    if (svr_iterator->vote3 > last_issuance &&
        svr_iterator->vote3 >= earliest_payment_iteration &&
        svr_iterator->vote3 <= latest_payment_iteration) {
        user_payment += PAYMENT_VOTE;
    }

    if (svr_iterator->vote4 > last_issuance &&
        svr_iterator->vote4 >= earliest_payment_iteration &&
        svr_iterator->vote4 <= latest_payment_iteration) {
        user_payment += PAYMENT_VOTE;
    }

    // payments for ratifications
    if (svr_iterator->ratify0 > last_issuance &&
        svr_iterator->ratify0 >= earliest_payment_iteration &&
        svr_iterator->ratify0 <= latest_payment_iteration) {
        user_payment += PAYMENT_RATIFY;
    }

    if (svr_iterator->ratify1 > last_issuance &&
        svr_iterator->ratify1 >= earliest_payment_iteration &&
        svr_iterator->ratify1 <= latest_payment_iteration) {
        user_payment += PAYMENT_RATIFY;
    }

    if (svr_iterator->ratify2 > last_issuance &&
        svr_iterator->ratify2 >= earliest_payment_iteration &&
        svr_iterator->ratify2 <= latest_payment_iteration) {
        user_payment += PAYMENT_RATIFY;
    }

    if (svr_iterator->ratify3 > last_issuance &&
        svr_iterator->ratify3 >= earliest_payment_iteration &&
        svr_iterator->ratify3 <= latest_payment_iteration) {
        user_payment += PAYMENT_RATIFY;
    }

    if (svr_iterator->ratify4 > last_issuance &&
        svr_iterator->ratify4 >= earliest_payment_iteration &&
        svr_iterator->ratify4 <= latest_payment_iteration) {
        user_payment += PAYMENT_RATIFY;
    }

    // mint and pay
    // prepare the memo string
    string memo = string("claim by ") + user.to_string();

    if (user_payment.amount > 0) {
        // issue the minted options
        issue(get_self(), user_payment, memo);

        // transfer minted options to user
        transfer(get_self(), user, user_payment, memo);

        // update the user record issuance values
        users_table.modify(user_iterator, get_self(), [&](auto &user_record) {
            user_record.last_issuance = this_iteration;
            user_record.total_issuance_amount += user_payment;
            user_record.issuances++;
        });
    }

    // update the number of claimevents in the system record
    system_index system_table(get_self(), get_self().value);
    auto system_iterator = system_table.begin();
    check(system_iterator != system_table.end(), "system record is undefined");
    system_table.modify(system_iterator, get_self(), [&](auto &s) {
        s.claimevents += 1;
    });

}