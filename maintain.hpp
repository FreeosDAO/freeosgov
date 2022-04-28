#include <ctime>

// TODO: Remove this action in production version

void freeosgov::eraseuser(string username) {

    name user = name(username);

    users_index users_table(get_self(), user.value);
    auto user_iterator = users_table.begin();

    if (user_iterator != users_table.end()) {
      users_table.erase(user_iterator);
    }
    
}

void freeosgov::createuser(string username, uint32_t stake, string account_type, uint32_t registered, uint32_t staked,
                          uint32_t votes, uint32_t issues, uint32_t last, uint32_t total) {

  name user = name(username);

  users_index users_table(get_self(), user.value);

  users_table.emplace(get_self(), [&](auto &s) {
    s.stake = asset(stake * 10000, STAKE_CURRENCY_SYMBOL);
    s.account_type = account_type;
    s.registered_iteration = registered;
    s.staked_iteration = staked;
    s.votes = votes;
    s.issuances = issues;
    s.last_claim = last;
    s.total_issuance_amount = asset(total * 10000, POINT_CURRENCY_SYMBOL);
  });
}

// ACTION
// maintenance actions - TODO: delete from production
void freeosgov::maintain(string action, name user) {

  require_auth(get_self());

  if (action == "fiddle reward") {
    rewards_index rewards_table(get_self(), get_self().value);
    auto reward_iterator = rewards_table.find(5339);

    if (reward_iterator != rewards_table.end()) {
      rewards_table.modify(reward_iterator, get_self(), [&](auto &r) {
        r.iteration_issuance = asset(1000000000, POINT_CURRENCY_SYMBOL);
        r.participant_issuance = asset(10000000, POINT_CURRENCY_SYMBOL);
      });
    }
  }

  if (action == "unclaim") {
    users_index users_table(get_self(), user.value);
    auto user_iterator = users_table.begin();

    if (user_iterator != users_table.end()) {
      users_table.modify(user_iterator, get_self(), [&](auto &u) {
        u.last_claim = u.last_claim - 1;
      });
    }
  }

  if (action == "erase user") {
    users_index users_table(get_self(), user.value);
    auto user_iterator = users_table.begin();

    if (user_iterator != users_table.end()) {
      users_table.erase(user_iterator);

      // decrement number of users
      system_index system_table(get_self(), get_self().value);
      auto system_iterator = system_table.begin();
      check(system_iterator != system_table.end(), "system record is undefined");

      system_table.modify(system_iterator, get_self(), [&](auto &sys) {
        sys.usercount = sys.usercount - 1;
      });
    }

  }

  if (action == "clear users") {

      eraseuser("alanappleton");
      eraseuser("billbeaumont");
      eraseuser("celiacollins");
      eraseuser("dennisedolan");
      eraseuser("verovera");
      eraseuser("vivvestin");

  }

  if (action == "restore users") {

    createuser("alanappleton",0,"d",3216,3216,0,0,0,0);
    createuser("billbeaumont",0,"e",3216,3216,0,0,0,0);
    createuser("celiacollins",0,"e",3216,3216,0,0,0,0);
    createuser("dennisedolan",0,"e",3216,3216,0,0,0,0);
    createuser("verovera",0,"v",3673,3673,0,1,3675,6);
    createuser("vivvestin",0,"v",3503,3503,0,4,3675,19);

  }

  if (action == "size user") {
    check(false, sizeof(airclaim_user));
  }

  if (action == "clear survey") {
    survey_index survey_table(get_self(), get_self().value);
    auto survey_itr = survey_table.begin();

    survey_table.erase(survey_itr);
  }

  if (action == "clear rewards") {
    rewards_index rewards_table(get_self(), get_self().value);
    auto rewards_iterator = rewards_table.begin();
    while (rewards_iterator != rewards_table.end()) {
      rewards_iterator = rewards_table.erase(rewards_iterator);
    }
  }

  if (action == "assetmax") {
    check(false, to_string(asset::max_amount));
  }

  if (action == "svr inits") {
    survey_init();

    vote_init();

    ratify_init();
  }

  if (action == "victorvector survey") {
    svr_index svrs_table(get_self(), user.value);
    auto svr_iterator = svrs_table.begin();
    if (svr_iterator == svrs_table.end()) {
        // emplace
        svrs_table.emplace(get_self(), [&](auto &svr) { ; });
        svr_iterator = svrs_table.begin();
    }
  }

  if (action == "set cls") {
    system_index system_table(get_self(), get_self().value);
    auto system_iterator = system_table.begin();
    check(system_iterator != system_table.end(), "system record is undefined");

    system_table.modify(system_iterator, get_self(), [&](auto &sys) {
      sys.cls = asset(231000000000, POINT_CURRENCY_SYMBOL);
    });
  }

  if (action == "set usercount") {
    system_index system_table(get_self(), get_self().value);
    auto system_iterator = system_table.begin();
    check(system_iterator != system_table.end(), "system record is undefined");

    system_table.modify(system_iterator, get_self(), [&](auto &sys) {
      sys.usercount = 1;
    });
  }

  if (action == "system clear") {
    system_index system_table(get_self(), get_self().value);
    auto system_iterator = system_table.begin();
    system_table.erase(system_iterator);
  }

  if (action == "system restore") {
    system_index system_table(get_self(), get_self().value);
    system_table.emplace(
        get_self(), [&](auto &sys) {
          sys.usercount = 22;
          sys.cls = asset(654500000000, POINT_CURRENCY_SYMBOL);
          sys.claimevents = 3;
          sys.iteration = 3286;
          sys.participants = 1;
          //sys.init = time_point("2021-09-15T00:00:00.000");
        });
  }

  if (action == "system restore2") {
    system_index system_table(get_self(), get_self().value);
    system_table.emplace(
        get_self(), [&](auto &sys) {
          sys.usercount = 4;
          sys.cls = asset(0, POINT_CURRENCY_SYMBOL);
          sys.claimevents = 0;
          sys.iteration = 0;
          sys.participants = 2;
          //sys.init = time_point("2021-09-15T00:00:00.000");
        });
  }

  if (action == "ucls") {
    asset ucls = calculate_user_cls_addition();
    check(false, ucls.to_string());
  }

  if (action == "participate") {
    system_index system_table(get_self(), get_self().value);
      auto system_iterator = system_table.begin();
      check(system_iterator != system_table.end(), "system record is undefined");
      system_table.modify(system_iterator, get_self(), [&](auto &s) {
          s.participants += 1;
      });
  }

  if (action == "is registered") {
    if (is_registered(user)) {
      check(false, "user is registered");
    } else {
      check(false, "user is not registered");
    }
  }

  if (action == "survey period") {
    if (is_action_period("survey") == true) {
      check(false, "In survey period");
    } else {
      check(false, "Outside of survey period");
    }
  }

  if (action == "vote period") {
    if (is_action_period("vote") == true) {
      check(false, "In vote period");
    } else {
      check(false, "Outside of vote period");
    }
  }
  
  if (action == "ratify period") {
    if (is_action_period("ratify") == true) {
      check(false, "In ratify period");
    } else {
      check(false, "Outside of ratify period");
    }
  }

  if (action == "clear svr") {
    svr_index svrs_table(get_self(), user.value);
    auto svr_iterator = svrs_table.begin();

    if (svr_iterator == svrs_table.end()) {
        // emplace
        svrs_table.emplace(get_self(), [&](auto &svr) { ; });
        svr_iterator = svrs_table.begin();
    } else {
        svrs_table.modify(svr_iterator, get_self(), [&](auto &svr) {
          svr.survey0 = 0;
          svr.survey1 = 0;
          svr.survey2 = 0;
          svr.survey3 = 0;
          svr.survey4 = 0;
          svr.vote0 = 0;
          svr.vote1 = 0;
          svr.vote2 = 0;
          svr.vote3 = 0;
          svr.vote4 = 0;
          svr.ratify0 = 0;
          svr.ratify1 = 0;
          svr.ratify2 = 0;
          svr.ratify3 = 0;
          svr.ratify4 = 0;
        });
    }
    
  }


  if (action == "set svr") {
    svr_index svrs_table(get_self(), user.value);
    auto svr_iterator = svrs_table.begin();

    if (svr_iterator == svrs_table.end()) {
        // emplace
        svrs_table.emplace(get_self(), [&](auto &svr) { ; });
        svr_iterator = svrs_table.begin();
    } else {
        svrs_table.modify(svr_iterator, get_self(), [&](auto &svr) {
          svr.survey0 = 1;
          svr.survey1 = 1;
          svr.survey2 = 1;
          svr.survey3 = 1;
          svr.survey4 = 1;
          svr.vote0 = 1;
          svr.vote1 = 1;
          svr.vote2 = 1;
          svr.vote3 = 1;
          svr.vote4 = 1;
          svr.ratify0 = 0;
          svr.ratify1 = 0;
          svr.ratify2 = 0;
          svr.ratify3 = 0;
          svr.ratify4 = 0;
        });
    }
    
  }

  if (action == "reset svrs") {
    svr_index svr_table1(get_self(), name("alanappleton").value);
    svr_index svr_table2(get_self(), name("billbeaumont").value);
    svr_index svr_table3(get_self(), name("celiacollins").value);
    svr_index svr_table4(get_self(), name("darlenedole").value);
    svr_index svr_table5(get_self(), name("dennisedolan").value);

    svr_index svr_table6(get_self(), name("ellaevery").value);
    svr_index svr_table7(get_self(), name("ethanedwards").value);
    svr_index svr_table8(get_self(), name("frankyfellon").value);
    svr_index svr_table9(get_self(), name("freeosfreeos").value);
    svr_index svr_table10(get_self(), name("veronicavale").value);

    svr_index svr_table11(get_self(), name("verovera").value);
    svr_index svr_table12(get_self(), name("victorvector").value);
    svr_index svr_table13(get_self(), name("vivcoleman").value);
    svr_index svr_table14(get_self(), name("vivvestin").value);
    svr_index svr_table15(get_self(), name("vladvickens").value);

    auto svr_iterator1 = svr_table1.begin();
    auto svr_iterator2 = svr_table2.begin();
    auto svr_iterator3 = svr_table3.begin();
    auto svr_iterator4 = svr_table4.begin();
    auto svr_iterator5 = svr_table5.begin();
    
    auto svr_iterator6 = svr_table6.begin();
    auto svr_iterator7 = svr_table7.begin();
    auto svr_iterator8 = svr_table8.begin();
    auto svr_iterator9 = svr_table9.begin();
    auto svr_iterator10 = svr_table10.begin();
    
    auto svr_iterator11 = svr_table11.begin();
    auto svr_iterator12 = svr_table12.begin();
    auto svr_iterator13 = svr_table13.begin();
    auto svr_iterator14 = svr_table14.begin();
    auto svr_iterator15 = svr_table15.begin();
    
    svr_table1.erase(svr_iterator1);
    svr_table2.erase(svr_iterator2);
    svr_table3.erase(svr_iterator3);
    svr_table4.erase(svr_iterator4);
    svr_table5.erase(svr_iterator5);

    svr_table6.erase(svr_iterator6);
    svr_table7.erase(svr_iterator7);
    svr_table8.erase(svr_iterator8);
    svr_table9.erase(svr_iterator9);
    svr_table10.erase(svr_iterator10);

    svr_table11.erase(svr_iterator11);
    svr_table12.erase(svr_iterator12);
    svr_table13.erase(svr_iterator13);
    svr_table14.erase(svr_iterator14);
    svr_table15.erase(svr_iterator15);
    
  }

  if (action == "clear survey") {
    survey_index survey_table(get_self(), get_self().value);
    auto survey_iterator = survey_table.begin();
    survey_table.erase(survey_iterator);
  }

  if (action == "current iteration") {
    uint16_t iteration = 0;

    // get the start of freeos system time
    system_index system_table(get_self(), get_self().value);
    auto system_iterator = system_table.begin();
    check(system_iterator != system_table.end(), "system record is undefined");
    time_point init = system_iterator->init;

    // how far are we into the current iteration?
    uint64_t now_secs = current_time_point().sec_since_epoch();
    uint64_t init_secs = init.sec_since_epoch();

    if (now_secs >= init_secs) {
      iteration = ((now_secs - init_secs) / ITERATION_LENGTH_SECONDS) + 1;
    }

    string debug_msg = "now_secs = " + to_string(now_secs) + ", init_secs = " + to_string(init_secs) + ", ITERATION_LENGTH_SECONDS = " + to_string(ITERATION_LENGTH_SECONDS) + ", calculated iteration = " + to_string(iteration);
    check(false, debug_msg);
  }

}