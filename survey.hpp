//#pragma once
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include "freeosgov.hpp"
#include "tables.hpp"

using namespace eosio;
using namespace freedao;
using namespace std;

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

std::vector<int> parse_survey_ranges(string surveyranges) {
    
    // the surveyranges string looks like this: q2:1-48,q4:1-48
    std::vector<int> limits;

    std::vector<std::string> tokenlist = split(surveyranges, ",");

    std::vector q1_2_param = split(tokenlist[0], ":");
    std::vector q2_2_param = split(tokenlist[1], ":");

    std::vector q1_minmax = split(q1_2_param[1], "-");
    std::vector q2_minmax = split(q2_2_param[1], "-");

    limits.push_back (stoi(q1_minmax[0]));
    limits.push_back (stoi(q1_minmax[1]));
    limits.push_back (stoi(q2_minmax[0]));
    limits.push_back (stoi(q2_minmax[1]));

    return limits;
}

void freeosgov::survey_init() {
    survey_index survey_table(get_self(), get_self().value);
    auto survey_iterator = survey_table.begin();

    if (survey_iterator == survey_table.end()) {
        // emplace
        survey_table.emplace(get_self(), [&](auto &s) { s.iteration = current_iteration(); });
    }    
}

// reset the survey record, ready for the new iteration
void freeosgov::survey_reset() {
    survey_index survey_table(get_self(), get_self().value);
    auto survey_iterator = survey_table.begin();

    if (survey_iterator != survey_table.end()) {
        survey_table.modify(survey_iterator, get_self(), [&](auto &survey) {
            survey.iteration = current_iteration();
            survey.participants = 0;
            survey.q1choice1 = 0;
            survey.q1choice2 = 0;
            survey.q1choice3 = 0;
            survey.q2average = 0.0;
            survey.q3choice1 = 0;
            survey.q3choice2 = 0;
            survey.q3choice3 = 0;
            survey.q4average = 0.0;
            survey.q5choice1 = 0;
            survey.q5choice2 = 0;
            survey.q5choice3 = 0;
            survey.q5choice4 = 0;
            survey.q5choice5 = 0;
            survey.q5choice6 = 0;
            survey.q5choice7 = 0;
            survey.q5choice8 = 0;
        });
    }
}

// ACTION
void freeosgov::survey(name user, uint8_t q1response, uint8_t q2response, uint8_t q3response, uint8_t q4response, uint8_t q5choice1, uint8_t q5choice2, uint8_t q5choice3) {
    
    require_auth(user);

    tick();

    // is the user registered?
    check(is_registered(user), "survey is not open to unregistered users");
    
    // is the user verified?
    check(is_user_verified(user), "survey is not open to unverified users");
    
    // is the system operational?
    uint32_t this_iteration = current_iteration();
    check(this_iteration != 0, "The freeos system is not available at this time");

    // is the user alive
    check(is_user_alive(user), "user has exceeded the maximum number of iterations");

    // are we in the survey period?
    check(is_action_period("survey"), "It is outside of the survey period");

    // has the user already completed the survey?
    svr_index svrs_table(get_self(), user.value);
    auto svr_iterator = svrs_table.begin();

    // if there is no svr record for the user then create it - we will update it at the end of the action
    if (svr_iterator == svrs_table.end()) {
        // emplace
        svrs_table.emplace(get_self(), [&](auto &svr) { ; });
        svr_iterator = svrs_table.begin();
    } else {
        check(svr_iterator->survey0 != this_iteration &&
            svr_iterator->survey1 != this_iteration &&
            svr_iterator->survey2 != this_iteration &&
            svr_iterator->survey3 != this_iteration &&
            svr_iterator->survey4 != this_iteration,
            "user has already completed the survey");
    }

    // parameter checking
    // get and parse the survey slider ranges
    string surveyranges = get_parameter(name("surveyranges"));
    std::vector<int> range_values = parse_survey_ranges(surveyranges);

    // responses 1 to 4 - must be an integer between bounds
    check(q1response >= 1 && q1response <= 3,   "Response 1 must be a number between 1 and 3");
    check(q2response >= range_values[0] && q2response <= range_values[1],  "Response 2 is out of range");
    check(q3response >= 1 && q3response <= 3,   "Response 3 must be a number between 1 and 3");
    check(q4response >= range_values[2] && q4response <= range_values[3],  "Response 4 is out of range");
    check(q5choice1 >= 1 && q5choice1 <= 6,     "Response 5 choice 1 must be a number between 1 and 6");
    check(q5choice2 >= 1 && q5choice2 <= 6,     "Response 5 choice 2 must be a number between 1 and 6");
    check(q5choice3 >= 1 && q5choice3 <= 6,     "Response 5 choice 3 must be a number between 1 and 6");

    // response 5 - the 3 choices must not contain duplicates
    check((q5choice1 != q5choice2) && (q5choice2 != q5choice3) && (q5choice3 != q5choice1), "Response 5 has duplicate values");

    // store the responses
    survey_index survey_table(get_self(), get_self().value);
    auto survey_iterator = survey_table.begin();
    check(survey_iterator != survey_table.end(), "survey record is not defined");

    // process the responses from the user
    // for multiple choice options, increment to add the user's selection
    // for running averages, compute new running average
    survey_table.modify(survey_iterator, get_self(), [&](auto &survey) {

        // question 1
        switch(q1response) {
        case 1:
            survey.q1choice1++;
            break;
        case 2:
            survey.q1choice2++;
            break;
        case 3:
            survey.q1choice3++;
            break;
        }

        // question 2 - running average
        survey.q2average = ((survey.q2average * survey.participants) + q2response) / (survey.participants + 1);

        // question 3
        switch(q3response) {
        case 1:
            survey.q3choice1++;
            break;
        case 2:
            survey.q3choice2++;
            break;
        case 3:
            survey.q3choice3++;
            break;
        }

        // question 4 - running average
        survey.q4average = ((survey.q4average * survey.participants) + q4response) / (survey.participants + 1);

        // question 5 - need to iterate the list of choices
        uint8_t q5choices[3] = { q5choice1, q5choice2, q5choice3 };
        uint8_t points[3] = { 3,2,1 };  // points to distribute for 1st, 2nd, 3rd priorities
        for (size_t i = 0; i < sizeof(q5choices); i++) {
            switch(q5choices[i]) {
                case 1:
                    survey.q5choice1 += points[i];
                    break;
                case 2:
                    survey.q5choice2 += points[i];
                    break;
                case 3:
                    survey.q5choice3 += points[i];
                    break;
                case 4:
                    survey.q5choice4 += points[i];
                    break;
                case 5:
                    survey.q5choice5 += points[i];
                    break;
                case 6:
                    survey.q5choice6 += points[i];
                    break;
            }
        }        

        // update the number of participants
        survey.participants += 1;

    }); // end of modify

    // record that the user has responded to this iteration's vote
    svrs_table.modify(svr_iterator, get_self(), [&](auto &svr) {
        switch (this_iteration % 5) {
            case 0: svr.survey0 = this_iteration; break;
            case 1: svr.survey1 = this_iteration; break;
            case 2: svr.survey2 = this_iteration; break;
            case 3: svr.survey3 = this_iteration; break;
            case 4: svr.survey4 = this_iteration; break;
        }
    }); // end of modify

    // increment the number of participants in this iteration
    system_index system_table(get_self(), get_self().value);
    auto system_iterator = system_table.begin();
    check(system_iterator != system_table.end(), "system record is undefined");
    system_table.modify(system_iterator, get_self(), [&](auto &s) {
        s.participants += 1;
    });

    // update the surveys counter in the user's participant record
    participants_index participants_table(get_self(), user.value);
    auto participant_iterator = participants_table.begin();
    check(participant_iterator != participants_table.end(), "participant record not found");
    participants_table.modify(participant_iterator, get_self(), [&](auto &p) {
        p.surveys += 1;
    });

}