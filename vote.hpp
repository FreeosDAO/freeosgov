//#pragma once
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include "freeosgov.hpp"
#include "tables.hpp"
#include "config.hpp"

using namespace eosio;
using namespace freedao;
using namespace std;

void freeosgov::initialise_vote() {
    vote_index vote_table(get_self(), get_self().value);
    auto vote_iterator = vote_table.begin();

    check(vote_iterator != vote_table.end(), "vote table is undefined");

    vote_table.modify(vote_iterator, _self, [&](auto &vote) {
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


std::vector<std::string> parseTokens(string s, string delimiter) {

    std::vector<std::string> tokenlist {};

    size_t pos = 0;
    std::string token;
    while ((pos = s.find(delimiter)) != std::string::npos) {
        token = s.substr(0, pos);
        tokenlist.push_back (token);
        s.erase(0, pos + delimiter.length());
    }
    tokenlist.push_back (token);

    return tokenlist;
}

vector<string> split (string s, string delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    string token;
    vector<string> res;

    while ((pos_end = s.find (delimiter, pos_start)) != string::npos) {
        token = s.substr (pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back (token);
    }

    res.push_back (s.substr (pos_start));
    return res;
}

std::vector<int> parse_ranges(string voteranges) {
    
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
void freeosgov::testranges() {
    // get and parse the vote slider ranges
    string voteranges = get_parameter(name("voteranges"));
    std::vector<int> value_ranges = parse_ranges(voteranges);

    string limits =
    to_string(value_ranges[0]) + " " +
    to_string(value_ranges[1]) + " " +
    to_string(value_ranges[2]) + " " +
    to_string(value_ranges[3]) + " " +
    to_string(value_ranges[4]) + " " +
    to_string(value_ranges[5]);

    check(false, limits);
}


// ACTION
void freeosgov::vote(name user, uint8_t q1response, uint8_t q2response, double q3response, string q4response, uint8_t q5response, uint8_t q6choice1, uint8_t q6choice2, uint8_t q6choice3) {
    
    require_auth(user);

    uint32_t this_iteration = current_iteration();
    
    // is the system operational?
    check(this_iteration != 0, "The freeos system is not available at this time");

    // is the user active
    check(is_user_active(user), "The user has exceeded the maximum number of iterations");

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
        check(svr_iterator->vote1 != this_iteration &&
            svr_iterator->vote2 != this_iteration &&
            svr_iterator->vote3 != this_iteration &&
            svr_iterator->vote4 != this_iteration,
            "user has already completed the vote");
    }

    // parameter checking

    // get and parse the vote slider ranges
    string voteranges = get_parameter(name("voteranges"));
    std::vector<int> range_values = parse_ranges(voteranges);

    // get the current price of Freeos
    exchange_index rates_table(get_self(), get_self().value);
    auto rate_iterator = rates_table.begin();
    check(rate_iterator != rates_table.end(), "current price of Freeos is undefined");
    double current_price = rate_iterator->currentprice;
    
    check(q1response >= range_values[0] && q1response <= range_values[1],  "Response 1 is out of range");
    check(q2response >= range_values[2] && q2response <= range_values[3],  "Response 2 is out of range");
    check(q3response >= 0.0167 && q3response <= current_price,   "Response 3 is out of range");
    check(q4response != "POOL" && q4response != "BURN",  "Response 4 must be 'POOL' or 'BURN");
    check(q5response >= range_values[4] && q5response <= range_values[5],  "Response 5 is out of range");
    check(q6choice1 >= 1 && q6choice1 <= 6,     "Response 6 choice 1 must be a number between 1 and 6");
    check(q6choice2 >= 1 && q6choice2 <= 6,     "Response 6 choice 2 must be a number between 1 and 6");
    check(q6choice3 >= 1 && q6choice3 <= 6,     "Response 6 choice 3 must be a number between 1 and 6");

    // response 6 - the 3 choices must not contain duplicates
    check((q6choice1 != q6choice2) && (q6choice2 != q6choice3) && (q6choice3 != q6choice1), "Response 6 has duplicate values");

    // store the responses
    vote_index vote_table(get_self(), get_self().value);
    auto vote_iterator = vote_table.begin();

    // when run for the very first time, add the vote record if not already present
    if (vote_iterator == vote_table.end()) {
        vote_table.emplace(get_self(), [&](auto &vote) { vote.iteration = this_iteration; });
        vote_iterator = vote_table.begin();
    }

    check(vote_iterator != vote_table.end(), "vote record is not defined");

    // check if we are on a new iteration. If yes, then re-initialise the running values in the vote table
    if (vote_iterator->iteration != this_iteration) {
        initialise_vote();
    }

    // process the responses from the user
    // for multiple choice options, increment to add the user's selection
    // for running averages, compute new running average
    vote_table.modify(vote_iterator, _self, [&](auto &vote) {

        // set iteration
        vote.iteration = this_iteration;

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
        
        // question 6 - need to iterate the list of choices
        uint8_t q6choices[3] = { q6choice1, q6choice2, q6choice3 };
        uint8_t points[3] = { 3,2,1 };

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
        vote.participants++;

    }); // end of modify

    // record that the user has responded to this iteration's vote
    // find the oldest iteration value in the user's svr record (or first 0) and overwrite it
    uint32_t svr_iteration[4] = { svr_iterator->vote1,  svr_iterator->vote2, svr_iterator->vote3, svr_iterator->vote4 };
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
                svr.vote1 = this_iteration;
                break;
            case 1:
                svr.vote2 = this_iteration;
                break;
            case 2:
                svr.vote3 = this_iteration;
                break;
            case 3:
                svr.vote4 = this_iteration;
                break;
        }

    }); // end of modify

}