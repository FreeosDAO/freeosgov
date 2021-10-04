//#pragma once
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include "freeosgov.hpp"
#include "tables.hpp"

using namespace eosio;
using namespace freedao;
using namespace std;

void freeosgov::initialise_survey() {
    survey_index survey_table(get_self(), get_self().value);
    auto survey_iterator = survey_table.begin();

    check(survey_iterator != survey_table.end(), "survey table is undefined");

    survey_table.modify(survey_iterator, _self, [&](auto &survey) {
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

// ACTION
void freeosgov::surveyflow(name user, uint8_t q1response, uint8_t q2response, uint8_t q3response, uint8_t q4response, uint8_t q5choice1, uint8_t q5choice2, uint8_t q5choice3) {
    
    require_auth(user);

    uint32_t this_iteration = current_iteration();
    
    // is the system operational?
    check(this_iteration != 0, "The freeos system is not available at this time");

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
        check(svr_iterator->survey1 != this_iteration &&
            svr_iterator->survey2 != this_iteration &&
            svr_iterator->survey3 != this_iteration &&
            svr_iterator->survey4 != this_iteration,
            "user has already completed the survey");
    }

    // parameter checking
    // responses 1 to 4 - must be an integer between bounds
    check(q1response >= 1 && q1response <= 3,   "Response 1 must be a number between 1 and 3");
    check(q2response >= 1 && q2response <= 48,  "Response 2 must be a number between 1 and 48");
    check(q3response >= 1 && q3response <= 3,   "Response 3 must be a number between 1 and 3");
    check(q4response >= 1 && q4response <= 48,  "Response 4 must be a number between 1 and 48");
    check(q5choice1 >= 1 && q5choice1 <= 6,     "Response 5 choice 1 must be a number between 1 and 6");
    check(q5choice2 >= 1 && q5choice2 <= 6,     "Response 5 choice 2 must be a number between 1 and 6");
    check(q5choice3 >= 1 && q5choice3 <= 6,     "Response 5 choice 3 must be a number between 1 and 6");

    // response 5 - the 3 choices must not contain duplicates
    check((q5choice1 != q5choice2) && (q5choice2 != q5choice3) && (q5choice3 != q5choice1), "Response 5 has duplicate values");

    // store the responses
    survey_index survey_table(get_self(), get_self().value);
    auto survey_iterator = survey_table.begin();

    // when run for the very first time, add the survey record if not already present
    if (survey_iterator == survey_table.end()) {
        survey_table.emplace(get_self(), [&](auto &survey) { survey.iteration = this_iteration; });
        survey_iterator = survey_table.begin();
    }

    check(survey_iterator != survey_table.end(), "survey record is not defined");

    // check if we are on a new iteration. If yes, then re-initialise the running values in the survey table
    if (survey_iterator->iteration != this_iteration) {
        initialise_survey();
    }

    // process the responses from the user
    // for multiple choice options, increment to add the user's selection
    // for running averages, compute new running average
    survey_table.modify(survey_iterator, _self, [&](auto &survey) {

        // set iteration
        survey.iteration = this_iteration;

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
        for (size_t i = 0; i < sizeof(q5choices); i++) {
            switch(q5choices[i]) {
                case 1:
                    survey.q5choice1++;
                    break;
                case 2:
                    survey.q5choice2++;
                    break;
                case 3:
                    survey.q5choice3++;
                    break;
                case 4:
                    survey.q5choice4++;
                    break;
                case 5:
                    survey.q5choice5++;
                    break;
                case 6:
                    survey.q5choice6++;
                    break;
            }
        }        

        // update the number of participants
        survey.participants++;

    }); // end of modify

    // record that the user has responded to this iteration's survey
    // find the oldest iteration value in the user's svr record (or first 0) and overwrite it
    uint32_t svr_iteration[4] = { svr_iterator->survey1,  svr_iterator->survey2, svr_iterator->survey3, svr_iterator->survey4 };
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
                svr.survey1 = this_iteration;
                break;
            case 1:
                svr.survey2 = this_iteration;
                break;
            case 2:
                svr.survey3 = this_iteration;
                break;
            case 3:
                svr.survey4 = this_iteration;
                break;
        }

    }); // end of modify

}