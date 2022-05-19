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

    require_auth(user);

    tick();

    uint32_t this_iteration = current_iteration();
    
    // is the system operational?
    check(this_iteration != 0, "The freeos system is not available at this time");

    // is the user alive?
    check(is_user_alive(user), "user has exceeded the maximum number of iterations");

    // find the user's last claim
    users_index users_table(get_self(), user.value);
    auto user_iterator = users_table.begin();
    check(user_iterator != users_table.end(), "user registration record is undefined");
    uint32_t last_claim = user_iterator->last_claim;

    // find the user's svr record
    svr_index svr_table(get_self(), user.value);
    auto svr_iterator = svr_table.begin();
    check(svr_iterator != svr_table.end(), "user has not completed any votes or surveys");

    // check if user has already claimed in this iteration
    check(this_iteration != last_claim, "you cannot claim more than once in an iteration");

    // check that we are not in iteration 1 - payments can only be made retrospectively
    check(this_iteration > 1, "claims are for previous iterations only");

    // determine which range of iterations we are paying for
    uint32_t earliest_payment_iteration = this_iteration > 4 ? this_iteration - 4 : 1;
    uint32_t latest_payment_iteration = this_iteration - 1;

    // open the rewards table
    rewards_index rewards_table(get_self(), get_self().value);

    // get the S,V,R share values
    double survey_share = get_dparameter(name("surveyshare"));
    double vote_share = get_dparameter(name("voteshare"));
    double ratify_share = get_dparameter(name("ratifyshare"));

    double freedaoshare = get_dparameter(name("freedaoshare"));
    double partnershare = get_dparameter(name("partnershare"));

    // get the freedao and partners accounts
    name freedao_acct = name(get_parameter(name("freedaoacct")));
    name partners_acct = name(get_parameter(name("partnersacct")));

    // keep some counters
    asset   total_user_payment = asset(0, POINT_CURRENCY_SYMBOL);
    uint8_t total_issuances = 0;


    // determine each iteration's payments
    for (uint32_t iter = earliest_payment_iteration; iter <= latest_payment_iteration; iter++) {
        
        // has the user already claimed for this iteration?
        // developer note: this next statement could be incorporated into the for loop, but implementing this way for clarity
        if (iter <= last_claim) continue;

        // get the reward record for the iteration
        auto reward_iterator = rewards_table.find(iter);
        if (reward_iterator == rewards_table.end()) continue;   // no reward record so continue to next iteration

        // check if the vote was ratified, otherwise no payment to be made for that iteration
        if (reward_iterator->ratified == false) continue;

        // initialise a running total of points to pay
        uint64_t user_payment_amount = 0;

        // initialise the memo
        string transfer_memo = "Claim for week " + to_string(iter) + ": ";

        // determine the total user payout for the iteration
        uint64_t iteration_reward_amount = reward_iterator->participant_issuance.amount;

        // split the total user payout according to whether S, V or R completed
        uint64_t payment_survey_amount = iteration_reward_amount * survey_share;
        uint64_t payment_vote_amount = iteration_reward_amount * vote_share;
        uint64_t payment_ratify_amount = iteration_reward_amount * ratify_share;

        // check if survey completed for this iteration
        transfer_memo += "Survey [" + to_string((int)(survey_share * 100)) + "%]: ";
        if (svr_iterator->survey0 == iter ||
            svr_iterator->survey1 == iter ||
            svr_iterator->survey2 == iter ||
            svr_iterator->survey3 == iter ||
            svr_iterator->survey4 == iter) {
                user_payment_amount += payment_survey_amount;
                transfer_memo += "(" + asset(payment_survey_amount, POINT_CURRENCY_SYMBOL).to_string() + ")";
            } else {
                transfer_memo += "(not completed)";
            }

        // check if vote completed for this iteration
        transfer_memo += ", Vote [" + to_string((int)(vote_share * 100)) + "%]: ";
        if (svr_iterator->vote0 == iter ||
            svr_iterator->vote1 == iter ||
            svr_iterator->vote2 == iter ||
            svr_iterator->vote3 == iter ||
            svr_iterator->vote4 == iter) {
                user_payment_amount += payment_vote_amount;
                transfer_memo += "(" + asset(payment_vote_amount, POINT_CURRENCY_SYMBOL).to_string() + ")";
            } else {
                transfer_memo += "(not completed)";
            }

        // check if ratify completed for this iteration
        transfer_memo += ", Ratify [" + to_string((int)(ratify_share * 100)) + "%]: ";
        if (svr_iterator->ratify0 == iter ||
            svr_iterator->ratify1 == iter ||
            svr_iterator->ratify2 == iter ||
            svr_iterator->ratify3 == iter ||
            svr_iterator->ratify4 == iter) {
                user_payment_amount += payment_ratify_amount;
                transfer_memo += "(" + asset(payment_ratify_amount, POINT_CURRENCY_SYMBOL).to_string() + ")";
            } else {
                transfer_memo += "(not completed)";
            }
        
        // make the payment, if any
        if (user_payment_amount > 0) {
            total_issuances++;

            // get the partner and freedao shares
            uint64_t freedao_payment_amount = user_payment_amount * freedaoshare;
            uint64_t partners_payment_amount = user_payment_amount * partnershare;

            // calculate the payments as asset
            asset user_payment = asset(user_payment_amount, POINT_CURRENCY_SYMBOL);
            asset freedao_payment = asset(freedao_payment_amount, POINT_CURRENCY_SYMBOL);
            asset partners_payment = asset(partners_payment_amount, POINT_CURRENCY_SYMBOL);

            total_user_payment += user_payment;

            // mint the total amount
            asset all_payments = user_payment + freedao_payment + partners_payment;
            string issue_memo = string("claim by ") + user.to_string() + " for week " + to_string(iter);
            
            // issue POINTs
            action issue_action = action(
                permission_level{get_self(), "active"_n}, get_self(),
                "mint"_n, std::make_tuple(get_self(), get_self(), all_payments, issue_memo));
            issue_action.send();

            // transfer POINTs to the user account
            action user_transfer_action = action(
            permission_level{get_self(), "active"_n}, get_self(),
                "allocate"_n,
                std::make_tuple(get_self(), user, user_payment, transfer_memo));
            user_transfer_action.send();

            // memo for freedao and partners shares
            string shares_memo = string("share of claim by ") + user.to_string() + " for week " + to_string(iter);

            // transfer POINTs to the freedao account
            if (freedao_payment_amount > 0) {
                action freedao_transfer_action = action(
                permission_level{get_self(), "active"_n}, get_self(),
                    "allocate"_n,
                    std::make_tuple(get_self(), freedao_acct, freedao_payment, shares_memo));
                freedao_transfer_action.send();
            }
            
            // transfer POINTs to the partners account
            if (partners_payment_amount > 0) {
                action partners_transfer_action = action(
                permission_level{get_self(), "active"_n}, get_self(),
                    "allocate"_n,
                    std::make_tuple(get_self(), partners_acct, partners_payment, shares_memo));
                partners_transfer_action.send();
            }
        }
    }

    // update the user record issuance values
    users_table.modify(user_iterator, get_self(), [&](auto &user_record) {
        user_record.last_claim = latest_payment_iteration;   // i.e. paid up to this iteration
        user_record.total_issuance_amount += total_user_payment;
        user_record.issuances += total_issuances;
    });

    // update the number of claimevents in the system record
    system_index system_table(get_self(), get_self().value);
    auto system_iterator = system_table.begin();
    check(system_iterator != system_table.end(), "system record is undefined");
    system_table.modify(system_iterator, get_self(), [&](auto &s) {
        s.claimevents += total_issuances;
    });

}