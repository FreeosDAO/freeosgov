//#pragma once
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include "freeosgov.hpp"
#include "tables.hpp"
#include "identity.hpp"

using namespace eosio;
using namespace freedao;
using namespace std;


[[eosio::action]]
void freeosgov::surveyandrew( name user, bool r0,  bool r1,  bool r2, // Question 1
                                uint8_t r3,                     // Question 2 - slider
                                bool r4,  bool r5,  bool r6,    // Question 3  
                                uint8_t r7,                     // Question 4 - slider
                                uint8_t r8,  uint8_t r9,  uint8_t r10,  // Question 5
                                uint8_t r11, uint8_t r12, uint8_t r13)
{
    require_auth(user);

    tick();

    uint32_t this_iteration = current_iteration();
    
    // is the system operational?
    check(this_iteration != 0, "The freeos system is not available at this time");

    // are we in the survey period?
    check(is_action_period("survey"), "It is outside of the survey period");

    // is the user verified?
    check(is_user_verified(user), "survey is not open to unverified users");

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

    //
    // Iteration Checking - is that new Survey??
    // 
        // Retrieve stored iteration number:
        globalres_index final_results(get_self(), get_self().value);
        uint64_t p_key=20; 
        auto iter = final_results.find(p_key); 
        uint32_t stored_iteration;
        stored_iteration = iter->gresult;
        //(pseudocode: it was initialized for that iteration already? if not initialize)
        if( stored_iteration != this_iteration ){ survey_init_andrew();
        } 

    // 
    // parameter checking
    //
        /**
     * In the table "final_results": 
     *    p_key 0 -   2 Question One   (three options).
     *    p_key 3       Question Two   (slider) - there is up to date average result.
     *    p_key 4  -  6 Question Three (three options).
     *    p_key 7       Question Four  (slider) - there is up to date average result.
     *    p_key 8  - 13 Question Five  (six options).
     *    p_key 14 - 16 Question Six   (three options). No Q6 - These rows are NOT USED
     *    p_key 17      Number of users submitted surveys up to date.
     *    p_key 18      Sum of all slider values for row 3 to count average.
     *    p_key 19      Sum of all slider values for row 7 to count average.  
     *    p_key 20      Stored iteration number - that means survey was initialized
     *              ... during that iteration. If current iteration number is different
     *              ... that means the final_results table is outdated and must be initialized. 
    */

    // get and parse the survey slider ranges
    string surveyranges = get_parameter(name("surveyranges"));
    std::vector<int> range_values = parse_survey_ranges(surveyranges);

        uint8_t q1 = 0;
        if (r0)
        {
            q1++;};
    if(r1){q1++;};
    if(r2){q1++;};
    check( q1==1, "First question not answered correctly");
    check( ((r3 >= range_values[0]) && (r3 <= range_values[1])), "Second question out of range");
    q1=0;
    if(r4){q1++;};
    if(r5){q1++;};
    if(r6){q1++;};
    check( q1==1, "Third question not answered correctly");
    check( ((r7 >= range_values[2]) && (r7 <= range_values[3])), "Fourth question out of range");
    q1=r8+r9+r10+r11+r12+r13;
    check( q1==6, "Fifth question not answered correctly");

    //
    // store responses and compute running averages
    // 
    // globalres_table final_results(get_self(), get_self().value);
    auto ite = final_results.begin(); 

    p_key=17; //Add user to the counter at position 17 of final_results table.
    iter = final_results.find(p_key); {final_results.modify(iter, get_self(),[&](auto &p)
        { p.gresult++; });};  
    double counter = iter->gresult;           // Duplicated final_results[17].
    //Total sum for sliders is going to final_results[18] and final_results[19] respectively:
    iter++; final_results.modify(iter, get_self(), [&](auto &p){p.gresult = p.gresult + r3;});
    double sum1 = iter->gresult;              // Duplicated final_results[18].
    iter++; final_results.modify(iter, get_self(), [&](auto &p){p.gresult = p.gresult + r7;});
    double sum2 = iter->gresult;              // Duplicated final_results[19].

    // Question 1: Select 1 of 3
    if(r0){final_results.modify(ite, get_self(),[&](auto &p){ p.gresult++; });} ite++;
    if(r1){final_results.modify(ite, get_self(),[&](auto &p){ p.gresult++; });} ite++;
    if(r2){final_results.modify(ite, get_self(),[&](auto &p){ p.gresult++; });} ite++;
    // Question 2: result between 0-50   //using r3
    {final_results.modify(ite, get_self(), [&](auto &p){p.gresult = sum1/counter;});} ite++;
    // Question 3: Select 1 of 3
    if(r4){final_results.modify(ite, get_self(),[&](auto &p){ p.gresult++; });} ite++;
    if(r5){final_results.modify(ite, get_self(),[&](auto &p){ p.gresult++; });} ite++;
    if(r6){final_results.modify(ite, get_self(),[&](auto &p){ p.gresult++; });} ite++; 
    // Question 4: result between 0-50   //using r7
    {final_results.modify(ite, get_self(),[&](auto &p){p.gresult = sum2/counter;});} ite++;
    // Question 5: Select 3 of 6 Note: r8-r13 contains priority points from front-end
    final_results.modify(ite, get_self(),[&](auto &p){ p.gresult = p.gresult + r8; }); ite++;
    final_results.modify(ite, get_self(),[&](auto &p){ p.gresult = p.gresult + r9; }); ite++;
    final_results.modify(ite, get_self(),[&](auto &p){ p.gresult = p.gresult + r10; }); ite++;
    final_results.modify(ite, get_self(),[&](auto &p){ p.gresult = p.gresult + r11; }); ite++;
    final_results.modify(ite, get_self(),[&](auto &p){ p.gresult = p.gresult + r12; }); ite++;
    final_results.modify(ite, get_self(),[&](auto &p){ p.gresult = p.gresult + r13; }); ite++;

    //
    // record that the user has responded to this iteration's survey
    size_t field_selector = this_iteration % 4;
    
    // write the current iteration into the appropriate field
    svrs_table.modify(svr_iterator, get_self(), [&](auto &svr) {
        switch (field_selector) {
            case 0: svr.survey1 = this_iteration; break;
            case 1: svr.survey2 = this_iteration; break;
            case 2: svr.survey3 = this_iteration; break;
            case 3: svr.survey4 = this_iteration; break;
        }
    }); // end of modify

    // increment the number of participants in this iteration
    system_index system_table(get_self(), get_self().value);
    auto system_iterator = system_table.begin();
    check(system_iterator != system_table.end(), "system record is undefined");
    system_table.modify(system_iterator, get_self(), [&](auto &s) {
        s.participants += 1;
    });

} // end of survey

//Manual init - first time //TODO Remove after TESTs
// [[eosio::action]]
// void freeosgov::init(){
//   require_auth(_self)
//   surveyinit();
// } //the end of TEST

//
//      ----- Functions used by the action survey: -----
//

/**
 * Function surveyinit
 * @details Function clean up the results table 
 * @pre  
 */

void freeosgov::survey_init_andrew(){
        globalres_index final_results(get_self(), get_self().value);
        uint32_t this_iteration = current_iteration();
        // All rows must be initialized to zero for all types of questions. 
        uint64_t pkey = 0;
        for(uint8_t i=0; i<21; i++) 
        {
         auto idx = final_results.find( pkey ); 
            if( idx == final_results.end() )
            {
              final_results.emplace( get_self(), [&]( auto& row ){
                row.p_key = final_results.available_primary_key(); 
                if(i==17){ row.gresult = 1; } else row.gresult=0; //count users
                if(i==20){ row.gresult = this_iteration; }}); //store this iter number         
            }
            else 
            { 
              final_results.modify(idx, get_self(), [&]( auto& row ) {  
                row.gresult = 0;         
              }); 
            } 
            pkey++;
        } //end of for

    } // end of surveyinit    