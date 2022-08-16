#include <ctime>
#include <eosio/asset.hpp>

using namespace eosio;
using namespace freedao;
using namespace std;

// TODO: Remove this action in production version

// set mintfeefree amount
void freeosgov::setmff(name user, asset amount) {
  require_auth(get_self());

  auto sym = amount.symbol;
  check(sym == POINT_CURRENCY_SYMBOL, "invalid symbol name, please specify amount in POINTs");

  mintfeefree_index mintfeefree_table(get_self(), user.value);
  auto user_iterator = mintfeefree_table.begin();

  if (user_iterator != mintfeefree_table.end()) {
    // modify
    mintfeefree_table.modify(user_iterator, get_self(), [&](auto &m) {
      m.balance = amount;
    });
  } else {
    // emplace
    mintfeefree_table.emplace(get_self(), [&](auto &m) {
      m.balance = amount;
    });
  }

}


void freeosgov::eraseuser(string username) {

    name user = name(username);

    participants_index participants_table(get_self(), user.value);
    auto participant_iterator = participants_table.begin();

    if (participant_iterator != participants_table.end()) {
      participants_table.erase(participant_iterator);
    }
    
}

void freeosgov::createuser(string username, string account_type, uint32_t registered, uint32_t surveys,
                          uint32_t votes, uint32_t ratifys, uint32_t issues, uint32_t last_claim, asset total) {

  name user = name(username);

  participants_index participants_table(get_self(), user.value);

  participants_table.emplace(get_self(), [&](auto &s) {
    s.account_type = account_type;
    s.registered_iteration = registered;
    s.surveys = surveys;
    s.votes = votes;
    s.ratifys = ratifys;
    s.issuances = issues;
    s.last_claim = last_claim;
    s.total_issuance_amount = total;
  });
}

void freeosgov::refund_function(name user) {
  credit_index credit_table(get_self(), user.value);
  auto credit_iterator = credit_table.begin();

  check(credit_iterator != credit_table.end(), "credit record not found");
  asset credit = credit_iterator->balance;
  string credit_msg = "credit for " + user.to_string() + " = " + credit.to_string();
  check(false, credit_msg);
}

// ACTION
// maintenance actions - TODO: delete from production
void freeosgov::maintain(string action, name user) {

  require_auth(get_self());

  if (action == "locked points") {
    symbol point_sym = symbol("POINT", 4);
    lockaccounts locked_points_table(get_self(), user.value);
    auto locked_iterator = locked_points_table.find(point_sym.code().raw());

    if (locked_iterator == locked_points_table.end()) {
      locked_points_table.emplace(
        get_self(), [&](auto &l) {
          l.balance = asset(27770123456789, POINT_CURRENCY_SYMBOL);
        });
    } else {
      locked_points_table.modify(locked_iterator, get_self(), [&](auto &l) {
        l.balance = asset(27770123456789, POINT_CURRENCY_SYMBOL);
      });
    }
    
  }

  if (action == "liquid points") {
    symbol point_sym = symbol("POINT", 4);
    accounts points_table(get_self(), user.value);
    auto points_iterator = points_table.find(point_sym.code().raw());

    if (points_iterator == points_table.end()) {
      points_table.emplace(
        get_self(), [&](auto &l) {
          l.balance = asset(27770123456789, POINT_CURRENCY_SYMBOL);
        });
    } else {
      points_table.modify(points_iterator, get_self(), [&](auto &l) {
        l.balance = asset(27770123456789, POINT_CURRENCY_SYMBOL);
      });
    }
    
  }

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
    participants_index participants_table(get_self(), user.value);
    auto participant_iterator = participants_table.begin();

    if (participant_iterator != participants_table.end()) {
      participants_table.modify(participant_iterator, get_self(), [&](auto &u) {
        u.last_claim = u.last_claim - 1;
      });
    }
  }

  if (action == "set iteration") {
    system_index system_table(get_self(), get_self().value);
    auto system_iterator = system_table.begin();
    check(system_iterator != system_table.end(), "system record not found");

    system_table.modify(system_iterator, get_self(), [&](auto &sys) {
      sys.iteration = 460617;
    });

    // delete rewards record if found
    rewards_index rewards_table(get_self(), get_self().value);
    auto reward_iterator = rewards_table.find(460617);
    if (reward_iterator != rewards_table.end()) {
      rewards_table.erase(reward_iterator);
    }
  }

  if (action == "erase old user") {
    airclaim_users_index oldusers_table(get_self(), user.value);
    auto olduser_iterator = oldusers_table.begin();

    if (olduser_iterator != oldusers_table.end()) {
      oldusers_table.erase(olduser_iterator);
    }
  }

  if (action == "erase user") {
    participants_index participants_table(get_self(), user.value);
    auto participant_iterator = participants_table.begin();

    if (participant_iterator != participants_table.end()) {
      participants_table.erase(participant_iterator);

      // decrement number of users
      system_index system_table(get_self(), get_self().value);
      auto system_iterator = system_table.begin();
      check(system_iterator != system_table.end(), "system record is undefined");

      system_table.modify(system_iterator, get_self(), [&](auto &sys) {
        sys.usercount = sys.usercount - 1;
      });
    }

  }

  if (action == "status") {
    string status_msg = string("");

    // XPR
    status_msg = status_msg + "XPR: ";
    accounts xpr_table(name("eosio.token"), user.value);
    auto xpr_iterator = xpr_table.begin();

    if (xpr_iterator != xpr_table.end()) {
      status_msg = status_msg + xpr_iterator->balance.to_string() + ", ";
    } else {
      status_msg = status_msg + "None, ";
    }

    // XUSDC
    status_msg = status_msg + "\nXUSDC: ";
    accounts xusdc_table(name("xtokens"), user.value);
    auto xusdc_iterator = xusdc_table.begin();

    if (xusdc_iterator != xusdc_table.end()) {
      status_msg = status_msg + xusdc_iterator->balance.to_string() + ", ";
    } else {
      status_msg = status_msg + "None, ";
    }

    // CREDITS
    status_msg = status_msg + "\nCREDITs: ";
    credit_index credit_table(get_self(), user.value);
    auto credit_iterator = credit_table.begin();

    string credits_str = string("");
    while (credit_iterator != credit_table.end()) {
      credits_str = credits_str + credit_iterator->balance.to_string() + ", ";
      credit_iterator++;
    }

    if (credits_str.length() == 0) {
      credits_str = string("None, ");
    }

    status_msg = status_msg + credits_str;

    // POINTs
    status_msg = status_msg + "\nPOINTs: ";
    accounts points_table(get_self(), user.value);
    auto points_iterator = points_table.begin();

    if (points_iterator != points_table.end()) {
      status_msg = status_msg + points_iterator->balance.to_string() + ", ";
    } else {
      status_msg = status_msg + "None, ";
    }

    // locked POINTs
    status_msg = status_msg + "\nLocked POINTs: ";
    lockaccounts locked_points_table(get_self(), user.value);
    auto locked_points_iterator = locked_points_table.begin();

    if (locked_points_iterator != locked_points_table.end()) {
      status_msg = status_msg + locked_points_iterator->balance.to_string() + ", ";
    } else {
      status_msg = status_msg + "None, ";
    }

    // MFF allowance
    status_msg = status_msg + "\nMFF: ";
    mintfeefree_index mff_table(get_self(), user.value);
    auto mff_iterator = mff_table.begin();

    if (mff_iterator != mff_table.end()) {
      status_msg = status_msg + mff_iterator->balance.to_string() + ", ";
    } else {
      status_msg = status_msg + "None, ";
    }

    // FREEBI
    status_msg = status_msg + "\nFREEBI: ";
    accounts freebi_table(name(freebi_acct), user.value);
    auto freebi_iterator = freebi_table.begin();

    if (freebi_iterator != freebi_table.end()) {
      status_msg = status_msg + freebi_iterator->balance.to_string() + ", ";
    } else {
      status_msg = status_msg + "None, ";
    }

    // FREEOS
    status_msg = status_msg + "\nFREEOS: ";
    accounts freeos_table(name(freeos_acct), user.value);
    auto freeos_iterator = freeos_table.begin();

    if (freeos_iterator != freeos_table.end()) {
      status_msg = status_msg + freeos_iterator->balance.to_string();
    } else {
      status_msg = status_msg + "None";
    }
    
    // Output
    check(false, status_msg);

  }



  if (action == "supplies") {
    string status_msg = string("");

    // XPR
    status_msg = status_msg + "\nXPR: ";
    accounts xpr_table(name("eosio.token"), get_self().value);
    auto xpr_iterator = xpr_table.begin();

    if (xpr_iterator != xpr_table.end()) {
      status_msg = status_msg + xpr_iterator->balance.to_string() + ", ";
    } else {
      status_msg = status_msg + "None, ";
    }

    // XUSDC
    status_msg = status_msg + "\nXUSDC: ";
    accounts xusdc_table(name("xtokens"), get_self().value);
    auto xusdc_iterator = xusdc_table.begin();

    if (xusdc_iterator != xusdc_table.end()) {
      status_msg = status_msg + xusdc_iterator->balance.to_string() + ", ";
    } else {
      status_msg = status_msg + "None, ";
    }

    // POINTs
    status_msg = status_msg + "\nPOINTs: ";
    accounts points_table(get_self(), get_self().value);
    auto points_iterator = points_table.begin();

    if (points_iterator != points_table.end()) {
      status_msg = status_msg + points_iterator->balance.to_string() + ", ";
    } else {
      status_msg = status_msg + "None, ";
    }

    // FREEBI
    status_msg = status_msg + "\nFREEBI: ";
    accounts freebi_table(name(freebi_acct), get_self().value);
    auto freebi_iterator = freebi_table.begin();

    if (freebi_iterator != freebi_table.end()) {
      status_msg = status_msg + freebi_iterator->balance.to_string() + ", ";
    } else {
      status_msg = status_msg + "None, ";
    }

    // FREEOS
    status_msg = status_msg + "\nFREEOS: ";
    accounts freeos_table(name(freeos_acct), user.value);
    auto freeos_iterator = freeos_table.begin();

    if (freeos_iterator != freeos_table.end()) {
      status_msg = status_msg + freeos_iterator->balance.to_string() + ", ";
    } else {
      status_msg = status_msg + "None, ";
    }

    // supply POINT
    status_msg = status_msg + "\nPOINT supply: ";
    symbol point_sym = symbol("POINT", 4);
    stats point_stat_table(get_self(), point_sym.code().raw());
    auto point_stat_iterator = point_stat_table.find(point_sym.code().raw());
    
    if (point_stat_iterator != point_stat_table.end()) {
      status_msg = status_msg + point_stat_iterator->supply.to_string() + ", ";
    } else {
      status_msg = status_msg + "None, ";
    }

    // supply FREEBI
    status_msg = status_msg + "\nFREEBI supply: ";
    symbol freebi_sym = symbol("FREEBI", 4);
    stats freebi_stat_table(name(freebi_acct), freebi_sym.code().raw());
    auto freebi_stat_iterator = freebi_stat_table.find(freebi_sym.code().raw());
    
    if (freebi_stat_iterator != freebi_stat_table.end()) {
      status_msg = status_msg + freebi_stat_iterator->supply.to_string() + ", ";
    } else {
      status_msg = status_msg + "None, ";
    }

    // supply FREEOS
    status_msg = status_msg + "\nFREEOS supply: ";
    symbol freeos_sym = symbol("FREEOS", 4);
    stats freeos_stat_table(name(freeos_acct), freeos_sym.code().raw());
    auto freeos_stat_iterator = freeos_stat_table.find(freeos_sym.code().raw());
    
    if (freeos_stat_iterator != freeos_stat_table.end()) {
      status_msg = status_msg + freeos_stat_iterator->supply.to_string() + ", ";
    } else {
      status_msg = status_msg + "None, ";
    }

    // Output
    check(false, status_msg);

  }
  
  if (action == "user credit XPR") {

    symbol xpr_sym = symbol("XPR", 4);

    credit_index credit_table(get_self(), user.value);
    auto credit_iterator = credit_table.find(xpr_sym.code().raw());

    check(credit_iterator != credit_table.end(), "credit record not found");
    asset credit = credit_iterator->balance;
    string credit_msg = "credit for " + user.to_string() + " = " + credit.to_string();
    check(false, credit_msg);
  }

  if (action == "user credit FREEBI") {

    symbol xpr_sym = symbol("FREEBI", 4);

    credit_index credit_table(get_self(), user.value);
    auto credit_iterator = credit_table.find(xpr_sym.code().raw());

    check(credit_iterator != credit_table.end(), "credit record not found");
    asset credit = credit_iterator->balance;
    string credit_msg = "credit for " + user.to_string() + " = " + credit.to_string();
    check(false, credit_msg);
  }

  if (action == "clear user credit") {
    credit_index credit_table(get_self(), user.value);
    auto credit_iterator = credit_table.begin();

    check(credit_iterator != credit_table.end(), "credit record not found");
    credit_table.erase(credit_iterator);
  }

  if (action == "user credit function") {
    refund_function(user);
  }

  if (action == "last reward") {
    rewards_index rewards_table(get_self(), get_self().value);
    auto reward_iterator = rewards_table.rbegin();
    check(false, "latest reward is for iteration " + to_string(reward_iterator->iteration));
  }

  if (action == "clear participants") {

      eraseuser("bigvern");
      eraseuser("billbeaumont");
      eraseuser("celiacollins");
      eraseuser("deliadally");
      eraseuser("dennisedolan");
      eraseuser("smcpher1");
      eraseuser("vennievans");
      eraseuser("veronicavale");
      eraseuser("verovera");
      eraseuser("vickvindaloo");
      eraseuser("vivcoleman");      
      eraseuser("vivvestin");

  }

  if (action == "migrate") {
    // old users record
    old_users_index old_users_table(get_self(), user.value);
    auto old_user_iterator = old_users_table.begin();
    check(old_user_iterator != old_users_table.end(), "user not found in old users table");

    // new users table
    participants_index participants_table(get_self(), user.value);
    participants_table.emplace(
        get_self(), [&](auto &p) {
          p.account_type = old_user_iterator->account_type;
          p.registered_iteration = old_user_iterator->registered_iteration;
          p.issuances = old_user_iterator->issuances;
          p.total_issuance_amount = old_user_iterator->total_issuance_amount;
          p.votes = old_user_iterator->votes;
          p.last_claim = old_user_iterator->last_claim;
        });
  }
  
  if (action == "restore participants") {
    createuser("bigvern", "v", 460614, 3, 6, 4, 3, 460691, asset(122788996, POINT_CURRENCY_SYMBOL));
    createuser("billbeaumont", "e", 3216, 0, 0, 0, 0, 0, asset(0, POINT_CURRENCY_SYMBOL));
    createuser("celiacollins", "e", 3216, 0, 0, 0, 0, 0, asset(0, POINT_CURRENCY_SYMBOL));
    createuser("deliadally", "v", 460757, 0, 1, 0, 0, 0, asset(0, POINT_CURRENCY_SYMBOL));
    createuser("dennisedolan", "e", 3216, 0, 0, 0, 0, 0, asset(0, POINT_CURRENCY_SYMBOL));
    createuser("smcpher1", "v", 1529, 21, 19, 5, 1, 460942, asset(85760977, POINT_CURRENCY_SYMBOL));
    createuser("vennievans", "e", 5690, 0, 0, 0, 1, 5711, asset(4404400, POINT_CURRENCY_SYMBOL));
    createuser("veronicavale", "v", 460756, 4, 4, 4, 1, 460777, asset(96793509, POINT_CURRENCY_SYMBOL));
    createuser("verovera", "v", 3673, 10, 15, 10, 13, 461088, asset(162375366, POINT_CURRENCY_SYMBOL));
    createuser("vickvindaloo", "v", 5545, 0, 0, 0, 9, 459629, asset(23948788, POINT_CURRENCY_SYMBOL));
    createuser("vivcoleman", "v", 1569, 0, 1, 0, 0, 0, asset(0, POINT_CURRENCY_SYMBOL));
    createuser("vivvestin", "v", 3503, 5, 2, 1, 11, 459629, asset(29548929, POINT_CURRENCY_SYMBOL));

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
          sys.claimevents = 13;
          sys.iteration = 5910;
          sys.participants = 0;
          //sys.init = time_point(1631620800000000);
        });
  }

  if (action == "system restore2") {
    system_index system_table(get_self(), get_self().value);
    system_table.emplace(
        get_self(), [&](auto &sys) {
          sys.usercount = 8;
          sys.cls = asset(154000000000, POINT_CURRENCY_SYMBOL);
          sys.claimevents = 17;
          sys.iteration = 5910;
          sys.participants = 0;
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

  if (action == "delete stat") {
    stats statstable(get_self(), POINT_CURRENCY_SYMBOL.code().raw());
    auto existing = statstable.find(POINT_CURRENCY_SYMBOL.code().raw());
    statstable.erase(existing);
  }

  if (action == "restore freeosgov stat") {

    stats statstable(get_self(), POINT_CURRENCY_SYMBOL.code().raw());
    statstable.emplace(get_self(), [&](auto &s) {
      s.supply = asset(10077528598, POINT_CURRENCY_SYMBOL);
      s.max_supply = asset(3500000000000000000, POINT_CURRENCY_SYMBOL);
      s.issuer = name("freeosgov");
    });
  }

  if (action == "restore freeosgov2 stat") {

    stats statstable(get_self(), POINT_CURRENCY_SYMBOL.code().raw());
    statstable.emplace(get_self(), [&](auto &s) {
      s.supply = asset(10077528598, POINT_CURRENCY_SYMBOL);
      s.max_supply = asset(3500000000000000000, POINT_CURRENCY_SYMBOL);
      s.issuer = name("freeosgov2");
    });
  }

  if (action == "get freebi stat") {

    asset input_quantity = asset(10000, FREEBI_CURRENCY_SYMBOL);

    auto sym = input_quantity.symbol;
    check(sym == POINT_CURRENCY_SYMBOL || sym == FREEBI_CURRENCY_SYMBOL, "invalid currency for quantity");

    // Point at the right contract
    name token_contract;
    if (sym == POINT_CURRENCY_SYMBOL) {
      token_contract = name(get_self());
    } else {
      token_contract = name(freebi_acct);
    }

    check(input_quantity.is_valid(), "invalid quantity");
    check(input_quantity.amount > 0, "must mint a positive quantity");

    stats statstable(token_contract, sym.code().raw());
    auto existing = statstable.find(sym.code().raw());
    
    check(existing != statstable.end(), "token with symbol does not exist");
    const auto &st = *existing;
    check(false, st.issuer.to_string());
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

    // read the iteration length in seconds
    int iteration_length_secs = get_iparameter(name("iterationsec"));

    if (now_secs >= init_secs) {
      iteration = ((now_secs - init_secs) / iteration_length_secs) + 1;
    }

    string debug_msg = "now_secs = " + to_string(now_secs) + ", init_secs = " + to_string(init_secs) + ", Iternation Length (seconds) = " + to_string(iteration_length_secs) + ", calculated iteration = " + to_string(iteration);
    check(false, debug_msg);
  }

}