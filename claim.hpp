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

    // is the user verified?
    check(is_user_verified(user), "claiming is restricted to verified users");

    // find the user's svr record
    svr_index svr_table(get_self(), user.value);
    auto svr_iterator = svr_table.begin();
    check(svr_iterator != svr_table.end(), "user has not completed any votes or surveys");


    // get the last issuance
    uint32_t last_issuance = user_iterator->last_issuance;

    // keep a running count of points to pay
    asset user_payment = asset(0, POINT_CURRENCY_SYMBOL);

    // we only pay out for surveys within the Claim History Period (4 iterations)
    uint32_t lapsed_claim_iteration = this_iteration - 4;

    // payment for surveys
    if (svr_iterator->survey1 > last_issuance && svr_iterator->survey1 > lapsed_claim_iteration) {
        user_payment += PAYMENT_SURVEY;
    }

    if (svr_iterator->survey2 > last_issuance && svr_iterator->survey2 > lapsed_claim_iteration) {
        user_payment += PAYMENT_SURVEY;
    }

    if (svr_iterator->survey3 > last_issuance && svr_iterator->survey3 > lapsed_claim_iteration) {
        user_payment += PAYMENT_SURVEY;
    }

    if (svr_iterator->survey4 > last_issuance && svr_iterator->survey4 > lapsed_claim_iteration) {
        user_payment += PAYMENT_SURVEY;
    }

    // payments for votes
    if (svr_iterator->vote1 > last_issuance && svr_iterator->vote1 > lapsed_claim_iteration) {
        user_payment += PAYMENT_VOTE;
    }

    if (svr_iterator->vote2 > last_issuance && svr_iterator->vote2 > lapsed_claim_iteration) {
        user_payment += PAYMENT_VOTE;
    }

    if (svr_iterator->vote3 > last_issuance && svr_iterator->vote3 > lapsed_claim_iteration) {
        user_payment += PAYMENT_VOTE;
    }

    if (svr_iterator->vote4 > last_issuance && svr_iterator->vote4 > lapsed_claim_iteration) {
        user_payment += PAYMENT_VOTE;
    }

    // payments for ratifications
    if (svr_iterator->ratify1 > last_issuance && svr_iterator->ratify1 > lapsed_claim_iteration) {
        user_payment += PAYMENT_RATIFY;
    }

    if (svr_iterator->ratify2 > last_issuance && svr_iterator->ratify2 > lapsed_claim_iteration) {
        user_payment += PAYMENT_RATIFY;
    }

    if (svr_iterator->ratify3 > last_issuance && svr_iterator->ratify3 > lapsed_claim_iteration) {
        user_payment += PAYMENT_RATIFY;
    }

    if (svr_iterator->ratify4 > last_issuance && svr_iterator->ratify4 > lapsed_claim_iteration) {
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
        users_table.modify(user_iterator, _self, [&](auto &user_record) {
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