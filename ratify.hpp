//#pragma once
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include "freeosgov.hpp"
#include "tables.hpp"

using namespace eosio;
using namespace freedao;
using namespace std;


void freeosgov::ratify_init() {
    ratify_index ratify_table(get_self(), get_self().value);
    auto ratify_iterator = ratify_table.begin();

    if(ratify_iterator == ratify_table.end()) {
        // emplace
        ratify_table.emplace(get_self(), [&](auto &r) { ; });
    } else {
        // modify
        ratify_table.modify(ratify_iterator, get_self(), [&](auto &ratify) {
        ratify.iteration = current_iteration();
        ratify.participants = 0;
        ratify.ratified = 0;
        });
    }

}

// ACTION
void freeosgov::ratify(name user, bool ratify_vote) {
    
    require_auth(user);

    tick();

    check(is_registered(user), "user is not registered");

    uint32_t this_iteration = current_iteration();
    
    // is the system operational?
    check(this_iteration != 0, "The freeos system is not available at this time");

    // are we in the ratify period?
    check(is_action_period("ratify"), "It is outside of the ratification period");

    // is the user verified?
    check(is_user_verified(user), "ratification is not open to unverified users");

    // has the user met the requirement of voting then ratifying?
    svr_index svrs_table(get_self(), user.value);
    auto svr_iterator = svrs_table.begin();
    check(svr_iterator != svrs_table.end(), "user must have voted in order to ratify");

    // check if the user has voted - a requirement for ratification
    check(svr_iterator->vote1 == this_iteration ||
        svr_iterator->vote2 == this_iteration ||
        svr_iterator->vote3 == this_iteration ||
        svr_iterator->vote4 == this_iteration,
        "user must have voted in order to ratify");

    // check if the user has already ratified
    check(svr_iterator->ratify1 != this_iteration &&
        svr_iterator->ratify2 != this_iteration &&
        svr_iterator->ratify3 != this_iteration &&
        svr_iterator->ratify4 != this_iteration,
        "user has already ratified");


    // store the responses
    ratify_index ratify_table(get_self(), get_self().value);
    auto ratify_iterator = ratify_table.begin();

    // when run for the very first time, add the ratify record if not already present
    if (ratify_iterator == ratify_table.end()) {
        ratify_table.emplace(get_self(), [&](auto &ratify) { ratify.iteration = this_iteration; });
        ratify_iterator = ratify_table.begin();
    }

    check(ratify_iterator != ratify_table.end(), "ratify record is not defined");

    // check if we are on a new iteration. If yes, then re-initialise the running values in the ratify table
    if (ratify_iterator->iteration != this_iteration) {
        ratify_init();
    }

    // process the responses from the user
    ratify_table.modify(ratify_iterator, _self, [&](auto &ratify) {

        // set iteration
        ratify.iteration = this_iteration;

        // ratified?
        if (ratify_vote == true) {
            ratify.ratified++;
        }        

        // update the number of participants
        ratify.participants++;

    }); // end of modify
    
    // record that the user has ratified
    svrs_table.modify(svr_iterator, _self, [&](auto &svr) {
        switch (this_iteration % 4) {
            case 0: svr.ratify1 = this_iteration; break;
            case 1: svr.ratify2 = this_iteration; break;
            case 2: svr.ratify3 = this_iteration; break;
            case 3: svr.ratify4 = this_iteration; break;
        }
    }); // end of modify

}