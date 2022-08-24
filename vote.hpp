//#pragma once
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include "freeosgov.hpp"
#include "tables.hpp"
#include "config.hpp"

using namespace eosio;
using namespace freedao;
using namespace std;

void freeosgov::vote_init() {
    vote_index vote_table(get_self(), get_self().value);
    auto vote_iterator = vote_table.begin();

    if (vote_iterator == vote_table.end()) {
        // emplace
        vote_table.emplace(get_self(), [&](auto &v) { v.iteration = current_iteration(); });
    }
}

// rest the vote record, ready for the new iteration
void freeosgov::vote_reset() {
    vote_index vote_table(get_self(), get_self().value);
    auto vote_iterator = vote_table.begin();

    if (vote_iterator != vote_table.end()) {
        vote_table.modify(vote_iterator, get_self(), [&](auto &vote) {
            vote.iteration = current_iteration();
            vote.participants = 0;
            vote.q1average = 0.0;
            vote.q2average = 0.0;
            vote.q3average = 0.0;
            vote.q4choice1 = 0;   // POOL
            vote.q4choice2 = 0;   // BURN
            vote.q5average = 0.0;
            vote.q6choice1 = 0;
            vote.q6choice2 = 0;
            vote.q6choice3 = 0;
            vote.q6choice4 = 0;
            vote.q6choice5 = 0;
            vote.q6choice6 = 0;
        });
    }

}


std::vector<int> parse_vote_ranges(string voteranges) {
    
    // the voteranges string looks like this: q1:0-100,q2:6-30,q5:0-50
    std::vector<int> limits;

    std::vector<std::string> tokenlist = split(voteranges, ",");

    std::vector q1_param = split(tokenlist[0], ":");
    std::vector q2_param = split(tokenlist[1], ":");
    std::vector q5_param = split(tokenlist[2], ":");

    std::vector q1_minmax = split(q1_param[1], "-");
    std::vector q2_minmax = split(q2_param[1], "-");
    std::vector q5_minmax = split(q5_param[1], "-");

    limits.push_back (stoi(q1_minmax[0]));
    limits.push_back (stoi(q1_minmax[1]));
    limits.push_back (stoi(q2_minmax[0]));
    limits.push_back (stoi(q2_minmax[1]));
    limits.push_back (stoi(q5_minmax[0]));
    limits.push_back (stoi(q5_minmax[1]));

    return limits;
}

// ACTION
void freeosgov::vote(name user, uint8_t q1response, uint8_t q2response, double q3response, string q4response, uint8_t q5response, uint8_t q6choice1, uint8_t q6choice2, uint8_t q6choice3) {
    
    require_auth(user);

    tick();

    // check that system is operational (masterswitch parameter set to "1")
    check(check_master_switch(), MSG_FREEOS_SYSTEM_NOT_AVAILABLE);

    // is the user registered?
    check(is_registered(user), "voting is not open to unregistered users");
    
    // is the user registered and verified?
    check(is_user_verified(user), "voting is not open to unverified users");
    
    // is the system operational?
    uint32_t this_iteration = current_iteration();
    check(this_iteration != 0, "The freeos system is not available at this time");

    // is the user alive?
    check(is_user_alive(user), "user has exceeded the maximum number of iterations");

    // are we in the vote period?
    check(is_action_period("vote"), "It is outside of the vote period");

    // has the user already completed the vote?
    svr_index svrs_table(get_self(), user.value);
    auto svr_iterator = svrs_table.begin();

    // if there is no svr record for the user then create it - we will update it at the end of the action
    if (svr_iterator == svrs_table.end()) {
        // emplace
        svrs_table.emplace(get_self(), [&](auto &svr) { ; });
        svr_iterator = svrs_table.begin();
    } else {
        check(svr_iterator->vote0 != this_iteration &&
            svr_iterator->vote1 != this_iteration &&
            svr_iterator->vote2 != this_iteration &&
            svr_iterator->vote3 != this_iteration &&
            svr_iterator->vote4 != this_iteration,
            "user has already voted");
    }

    // parameter checking
    // n.b. split function is defined in survey.hpp

    // get and parse the vote slider ranges
    string voteranges = get_parameter(name("voteranges"));
    std::vector<int> vote_range_values = parse_vote_ranges(voteranges);

    // get the current price of Freeos
    exchange_index rates_table(get_self(), get_self().value);
    auto rate_iterator = rates_table.begin();
    check(rate_iterator != rates_table.end(), "current price of Freeos is undefined");
    double current_price = rate_iterator->currentprice;
    // double target_price = rate_iterator->targetprice;

    // calculate the upper bound of locking threshold (q3)
    double lock_factor = get_dparameter(name("lockfactor"));

    double locking_threshold_upper_limit = 0.0;
    if (current_price < HARD_EXCHANGE_RATE_FLOOR) {
        locking_threshold_upper_limit = lock_factor * HARD_EXCHANGE_RATE_FLOOR;
    } else {
        locking_threshold_upper_limit = lock_factor * current_price;
    }

    
    check(q1response >= vote_range_values[0] && q1response <= vote_range_values[1],  "Response 1 is out of range");
    check(q2response >= vote_range_values[2] && q2response <= vote_range_values[3],  "Response 2 is out of range");
    check(q3response >= HARD_EXCHANGE_RATE_FLOOR && q3response <= locking_threshold_upper_limit,   "Response 3 is out of range");
    check(q4response == "POOL" || q4response == "BURN",  "Response 4 must be 'POOL' or 'BURN'");
    check(q5response >= vote_range_values[4] && q5response <= vote_range_values[5],  "Response 5 is out of range");
    check(q6choice1 >= 1 && q6choice1 <= 6,     "Response 6 choice 1 must be a number between 1 and 6");
    check(q6choice2 >= 1 && q6choice2 <= 6,     "Response 6 choice 2 must be a number between 1 and 6");
    check(q6choice3 >= 1 && q6choice3 <= 6,     "Response 6 choice 3 must be a number between 1 and 6");

    // response 6 - the 3 choices must not contain duplicates
    check((q6choice1 != q6choice2) && (q6choice2 != q6choice3) && (q6choice3 != q6choice1), "Response 6 has duplicate values");

    // store the responses
    vote_index vote_table(get_self(), get_self().value);
    auto vote_iterator = vote_table.begin();
    check(vote_iterator != vote_table.end(), "vote record is not defined");

    // process the responses from the user
    // for multiple choice options, increment to add the user's selection
    // for running averages, compute new running average
    vote_table.modify(vote_iterator, get_self(), [&](auto &vote) {

        // question 1
        vote.q1average = ((vote.q1average * vote.participants) + q1response) / (vote.participants + 1);

        // question 2
        vote.q2average = ((vote.q2average * vote.participants) + q2response) / (vote.participants + 1);

        // question 3
        vote.q3average = ((vote.q3average * vote.participants) + q3response) / (vote.participants + 1);

        // question 4
        if (q4response == "POOL") {
            vote.q4choice1++;
        } else {
            vote.q4choice2++;
        }

        // question 5
        vote.q5average = ((vote.q5average * vote.participants) + q5response) / (vote.participants + 1);
        
        // question 6 - need to iterate the list of choices
        uint8_t q6choices[3] = { q6choice1, q6choice2, q6choice3 };
        uint8_t points[3] = { 3,2,1 };  // points to distribute for 1st, 2nd, 3rd choices

        for (size_t i = 0; i < sizeof(q6choices); i++) {
            switch(q6choices[i]) {
                case 1:
                    vote.q6choice1 += points[i];
                    break;
                case 2:
                    vote.q6choice2 += points[i];
                    break;
                case 3:
                    vote.q6choice3 += points[i];
                    break;
                case 4:
                    vote.q6choice4 += points[i];
                    break;
                case 5:
                    vote.q6choice5 += points[i];
                    break;
                case 6:
                    vote.q6choice6 += points[i];
                    break;
            }
        }        

        // update the number of participants
        vote.participants += 1;

    }); // end of modify

    // record that the user has responded to this iteration's vote
    uint32_t survey_completed = 0;
    svrs_table.modify(svr_iterator, get_self(), [&](auto &svr) {
        switch (this_iteration % 5) {
            case 0: svr.vote0 = this_iteration; survey_completed = svr.survey0; break;
            case 1: svr.vote1 = this_iteration; survey_completed = svr.survey1; break;
            case 2: svr.vote2 = this_iteration; survey_completed = svr.survey2; break;
            case 3: svr.vote3 = this_iteration; survey_completed = svr.survey3; break;
            case 4: svr.vote4 = this_iteration; survey_completed = svr.survey4; break;
        }
    }); // end of modify

    // increment the number of participants in this iteration...
    // ... unless they have completed survey, in which case they have already been counted
    if (survey_completed != this_iteration) {
        // increment the number of participants in this iteration
        system_index system_table(get_self(), get_self().value);
        auto system_iterator = system_table.begin();
        check(system_iterator != system_table.end(), "system record is undefined");
        system_table.modify(system_iterator, get_self(), [&](auto &s) {
            s.participants += 1;
        });
    }

    // update the votes counter in the user's participant record
    participants_index participants_table(get_self(), user.value);
    auto participant_iterator = participants_table.begin();
    check(participant_iterator != participants_table.end(), "participant record not found");
    participants_table.modify(participant_iterator, get_self(), [&](auto &p) {
        p.votes += 1;
    });

}