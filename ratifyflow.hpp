//#pragma once
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include "freeosgov.hpp"
#include "tables.hpp"

using namespace eosio;
using namespace freedao;
using namespace std;

// ACTION
void freeosgov::ratifyflow(name user, bool ratify) {
    
    require_auth(user);

    uint32_t this_iteration = current_iteration();
    
    // is the system operational?
    check(this_iteration != 0, "The freeos system is not available at this time");

    // are we in the ratify period?
    check(is_action_period("ratify"), "It is outside of the ratification period");

    // has the user already ratified?
    svr_index svrs_table(get_self(), user.value);
    auto svr_iterator = svrs_table.begin();

    // if there is no svr record for the user then create it - we will update it at the end of the action
    if (svr_iterator == svrs_table.end()) {
        // emplace
        svrs_table.emplace(get_self(), [&](auto &svr) { ; });
        svr_iterator = svrs_table.begin();
    } else {
        check(svr_iterator->ratify1 != this_iteration &&
            svr_iterator->ratify2 != this_iteration &&
            svr_iterator->ratify3 != this_iteration &&
            svr_iterator->ratify4 != this_iteration,
            "user has already ratified");
    }

    
    // record that the user has ratified
    // find the oldest iteration value in the user's svr record (or first 0) and overwrite it
    uint32_t svr_iteration[4] = { svr_iterator->ratify1,  svr_iterator->ratify2, svr_iterator->ratify3, svr_iterator->ratify4 };
    size_t   min_iteration_index = 0;
    uint32_t min_iteration = svr_iteration[0];
    for (size_t i = 0; i < 4; i++) {
        // have we found a 0 in the list? if so, stop there.
        if (svr_iteration[i] == 0) {
            min_iteration_index = i;
            break;
        }

        // find the smallest iteration value and record its position in the array
        if (svr_iteration[i] < min_iteration) {
            min_iteration = svr_iteration[i];
            min_iteration_index = i;
        }
    }

    // At this point min_iteration_index contains the position of the earliest iteration in the array
    // write the current iteration into the appropriate field
    svrs_table.modify(svr_iterator, _self, [&](auto &svr) {

        switch (min_iteration_index) {
            case 0:
                svr.ratify1 = this_iteration;
                break;
            case 1:
                svr.ratify2 = this_iteration;
                break;
            case 2:
                svr.ratify3 = this_iteration;
                break;
            case 3:
                svr.ratify4 = this_iteration;
                break;
        }

    }); // end of modify

}