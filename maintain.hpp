// TODO: Remove this action in production version

// ACTION
// maintenance actions - TODO: delete from production
void freeosgov::maintain(string action, name user) {

  require_auth(get_self());

  if (action == "erase user") {
    users_index users_table(get_self(), user.value);
    auto user_iterator = users_table.begin();

    if (user_iterator != users_table.end()) {
      users_table.erase(user_iterator);

      // decrement number of users
      system_index system_table(get_self(), get_self().value);
      auto system_iterator = system_table.begin();
      check(system_iterator != system_table.end(), "system record is undefined");

      system_table.modify(system_iterator, _self, [&](auto &sys) {
        sys.usercount = sys.usercount - 1;
      });
    }

  }

  if (action == "clear users") {
      users_index users_table_a(get_self(), name("analappleton").value);
      auto iterator_a = users_table_a.begin();
      users_table_a.erase(iterator_a);

      users_index users_table_b(get_self(), name("billbeaumont").value);
      auto iterator_b = users_table_b.begin();
      users_table_b.erase(iterator_b);

      users_index users_table_c(get_self(), name("celiacollins").value);
      auto iterator_c = users_table_c.begin();
      users_table_c.erase(iterator_c);

      users_index users_table_d(get_self(), name("dennisedolan").value);
      auto iterator_d = users_table_d.begin();
      users_table_d.erase(iterator_d);

      users_index users_table_e(get_self(), name("ethanedwards").value);
      auto iterator_e = users_table_e.begin();
      users_table_e.erase(iterator_e);

      users_index users_table_f(get_self(), name("frankyfellon").value);
      auto iterator_f = users_table_f.begin();
      users_table_f.erase(iterator_f);
  }

  if (action == "restore users") {
      // alanappleton
      users_index users_table_a(get_self(), name("analappleton").value);
      users_table_a.emplace(get_self(), [&](auto &s) {
        s.stake = asset(0, STAKE_CURRENCY_SYMBOL);
        //s.feefree_airclaim_points = asset(0, POINT_CURRENCY_SYMBOL);
        s.account_type = "e";
        s.registered_iteration = 1;
        s.staked_iteration = 1;
        s.votes = 0;
        s.issuances = 0;
        s.last_issuance = 2;
        s.total_issuance_amount = asset(120000, POINT_CURRENCY_SYMBOL);
      });

      // billbeaumont
      users_index users_table_b(get_self(), name("billbeaumont").value);
      users_table_b.emplace(get_self(), [&](auto &s) {
        s.stake = asset(0, STAKE_CURRENCY_SYMBOL);
        //s.feefree_airclaim_points = asset(0, POINT_CURRENCY_SYMBOL);
        s.account_type = "e";
        s.registered_iteration = 8;
        s.staked_iteration = 8;
        s.votes = 0;
        s.issuances = 0;
        s.last_issuance = 0;
        s.total_issuance_amount = asset(0, POINT_CURRENCY_SYMBOL);
      });

      // celiacollins
      users_index users_table_c(get_self(), name("celiacollins").value);
      users_table_c.emplace(get_self(), [&](auto &s) {
        s.stake = asset(0, STAKE_CURRENCY_SYMBOL);
        //s.feefree_airclaim_points = asset(0, POINT_CURRENCY_SYMBOL);
        s.account_type = "e";
        s.registered_iteration = 8;
        s.staked_iteration = 8;
        s.votes = 0;
        s.issuances = 0;
        s.last_issuance = 0;
        s.total_issuance_amount = asset(0, POINT_CURRENCY_SYMBOL);
      });

      // dennisedolan
      users_index users_table_d(get_self(), name("dennisedolan").value);
      users_table_c.emplace(get_self(), [&](auto &s) {
        s.stake = asset(0, STAKE_CURRENCY_SYMBOL);
        //s.feefree_airclaim_points = asset(0, POINT_CURRENCY_SYMBOL);
        s.account_type = "e";
        s.registered_iteration = 8;
        s.staked_iteration = 8;
        s.votes = 0;
        s.issuances = 0;
        s.last_issuance = 0;
        s.total_issuance_amount = asset(0, POINT_CURRENCY_SYMBOL);
      });

      // ethanedwards
      users_index users_table_e(get_self(), name("ethanedwards").value);
      users_table_c.emplace(get_self(), [&](auto &s) {
        s.stake = asset(0, STAKE_CURRENCY_SYMBOL);
        //s.feefree_airclaim_points = asset(0, POINT_CURRENCY_SYMBOL);
        s.account_type = "e";
        s.registered_iteration = 8;
        s.staked_iteration = 8;
        s.votes = 0;
        s.issuances = 0;
        s.last_issuance = 0;
        s.total_issuance_amount = asset(0, POINT_CURRENCY_SYMBOL);
      });

      // frankyfellon
      users_index users_table_f(get_self(), name("frankyfellon").value);
      users_table_c.emplace(get_self(), [&](auto &s) {
        s.stake = asset(0, STAKE_CURRENCY_SYMBOL);
        //s.mintfree_airclaim_points = asset(0, POINT_CURRENCY_SYMBOL);
        s.account_type = "e";
        s.registered_iteration = 8;
        s.staked_iteration = 8;
        s.votes = 0;
        s.issuances = 0;
        s.last_issuance = 0;
        s.total_issuance_amount = asset(0, POINT_CURRENCY_SYMBOL);
      });

  }

  if (action == "clear rewards") {
    rewards_index rewards_table(get_self(), get_self().value);
    auto rewards_iterator = rewards_table.begin();
    while (rewards_iterator != rewards_table.end()) {
      rewards_iterator = rewards_table.erase(rewards_iterator);
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

  if (action == "get user cls") {
    asset amount = asset(0, POINT_CURRENCY_SYMBOL);
    amount += UCLS; // add to the CLS for the verified user
    amount += PARTNER_CLS_ADDITION; // add to CLS for the partners
    check(false, amount.to_string());
  }

  if (action == "set usercount") {
    system_index system_table(get_self(), get_self().value);
    auto system_iterator = system_table.begin();
    check(system_iterator != system_table.end(), "system record is undefined");

    system_table.modify(system_iterator, _self, [&](auto &sys) {
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
          sys.usercount = 6;
          sys.cls = asset(231000000000, POINT_CURRENCY_SYMBOL);
          sys.claimevents = 0;
          sys.iteration = 9;
          // sys.particpants = 0;
          // sys.init = time_point("2021-09-15T00:00:00.000");
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
        svrs_table.modify(svr_iterator, _self, [&](auto &svr) {
          svr.survey1 = 0;
          svr.survey2 = 0;
          svr.survey3 = 0;
          svr.survey4 = 0;
          svr.vote1 = 0;
          svr.vote2 = 0;
          svr.vote3 = 0;
          svr.vote4 = 0;
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
        svrs_table.modify(svr_iterator, _self, [&](auto &svr) {
          svr.survey1 = 1;
          svr.survey2 = 1;
          svr.survey3 = 1;
          svr.survey4 = 1;
          svr.vote1 = 1;
          svr.vote2 = 1;
          svr.vote3 = 1;
          svr.vote4 = 1;
          svr.ratify1 = 0;
          svr.ratify2 = 0;
          svr.ratify3 = 0;
          svr.ratify4 = 0;
        });
    }
    
  }

}