#include <ctime>
#include <eosio/asset.hpp>
#include <cmath>
#include "identity.hpp"

using namespace eosio;
using namespace freedao;
using namespace std;

// TODO: Remove this action in production version

void freeosgov::calcfee(const name &from, const asset& transfer_quantity)
{
   asset fee = asset(0, symbol(FREEBI_CURRENCY_CODE, 4));   // default value
   
   name freeoscontract = name("freeosgov2");
   name freebicontract = name("freebi");

   // the transfer fee is only charged for user-to-user transfers - do not charge if the transfer is from a contract
   if (from != freeoscontract && from != freebicontract) {
      name fee_parameter = name("freebixfee");
   
      freedao::dparameters_index dparameters_table(freeoscontract, freeoscontract.value);
      auto dparameter_iterator = dparameters_table.find(fee_parameter.value);
      check(dparameter_iterator != dparameters_table.end(), fee_parameter.to_string() + " is not defined");
      double fee_percent = dparameter_iterator->value;

      int64_t fee_units = transfer_quantity.amount * (fee_percent / 100.0);
      fee = asset(fee_units, symbol(FREEBI_CURRENCY_CODE, 4));
   }

   asset recipient_receives = transfer_quantity - fee;   
   
   check(false, "transfer_quanity = " + to_string(transfer_quantity.amount) + ", fee = " + to_string(fee.amount) + ", recipient_recives = " + to_string(recipient_receives.amount));
}

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

  if (action == "clear rewards") {
    rewards_index rewards_table(get_self(), get_self().value);
    auto reward_iterator = rewards_table.begin();
    
    while (reward_iterator != rewards_table.end()) {
      reward_iterator = rewards_table.erase(reward_iterator);
    }
  }

  if (action == "restore rewards") {
    rewards_index rewards_table(get_self(), get_self().value);

    rewards_table.emplace(get_self(), [&](auto &r) {
      r.iteration = 461495,
      r.iteration_cls = asset(535671010708, POINT_CURRENCY_SYMBOL);
      r.iteration_issuance = asset(0, POINT_CURRENCY_SYMBOL);
      r.participant_issuance = asset(0, POINT_CURRENCY_SYMBOL);
      r.participants = 0;
      r.issuance_rate = 1.00000000000000000;
      r.mint_fee_percent = 6.00000000000000000;
      r.mint_fee_percent_xpr = 6.0;
      r.mint_fee_percent_xusdc = 6.0;
      r.locking_threshold = 0.01670000000000000;
      r.pool = 1;
      r.burn = 0;
      r.ratified = 0;
      double    mint_freebi_transfer_fee = 0.0;
      double    point_freeos_ratio = 1.0;
      double    mint_throttle = 0.0;
      bool    burn_to_boost = false;
      });

      rewards_table.emplace(get_self(), [&](auto &r) {
      r.iteration = 461496,
      r.iteration_cls = asset(535671010708, POINT_CURRENCY_SYMBOL);
      r.iteration_issuance = asset(0, POINT_CURRENCY_SYMBOL);
      r.participant_issuance = asset(0, POINT_CURRENCY_SYMBOL);
      r.participants = 0;
      r.issuance_rate = 1.00000000000000000;
      r.mint_fee_percent = 6.00000000000000000;
      r.mint_fee_percent_xpr = 6.0;
      r.mint_fee_percent_xusdc = 6.0;
      r.locking_threshold = 0.01670000000000000;
      r.pool = 1;
      r.burn = 0;
      r.ratified = 0;
      double    mint_freebi_transfer_fee = 0.0;
      double    point_freeos_ratio = 1.0;
      double    mint_throttle = 0.0;
      bool      burn_to_boost = false;
      });

      rewards_table.emplace(get_self(), [&](auto &r) {
      r.iteration = 461497,
      r.iteration_cls = asset(535671010708, POINT_CURRENCY_SYMBOL);
      r.iteration_issuance = asset(0, POINT_CURRENCY_SYMBOL);
      r.participant_issuance = asset(0, POINT_CURRENCY_SYMBOL);
      r.participants = 0;
      r.issuance_rate = 1.00000000000000000;
      r.mint_fee_percent = 6.00000000000000000;
      r.mint_fee_percent_xpr = 6.0;
      r.mint_fee_percent_xusdc = 6.0;
      r.locking_threshold = 0.01670000000000000;
      r.pool = 1;
      r.burn = 0;
      r.ratified = 0;
      double    mint_freebi_transfer_fee = 0.0;
      double    point_freeos_ratio = 1.0;
      double    mint_throttle = 0.0;
      bool      burn_to_boost = false;
      });

      rewards_table.emplace(get_self(), [&](auto &r) {
      r.iteration = 461498,
      r.iteration_cls = asset(535671010708, POINT_CURRENCY_SYMBOL);
      r.iteration_issuance = asset(0, POINT_CURRENCY_SYMBOL);
      r.participant_issuance = asset(0, POINT_CURRENCY_SYMBOL);
      r.participants = 0;
      r.issuance_rate = 1.00000000000000000;
      r.mint_fee_percent = 6.00000000000000000;
      r.mint_fee_percent_xpr = 6.0;
      r.mint_fee_percent_xusdc = 6.0;
      r.locking_threshold = 0.01670000000000000;
      r.pool = 1;
      r.burn = 0;
      r.ratified = 0;
      double    mint_freebi_transfer_fee = 0.0;
      double    point_freeos_ratio = 1.0;
      double    mint_throttle = 0.0;
      bool      burn_to_boost = false;
      });
  }
  
  if (action == "clear voterecord") {
    vote_index voterecord_table(get_self(), get_self().value);
    auto voterecord_iterator = voterecord_table.begin();
    check(voterecord_iterator != voterecord_table.end(), "voterecord not found");

    voterecord_table.erase(voterecord_iterator);
  }

  if (action == "restore voterecord") {
    vote_index voterecord_table(get_self(), get_self().value);

    voterecord_table.emplace(get_self(), [&](auto &v) { v.iteration = 461498; });
  }

  if (action == "diagnose unlock") {

    // calculate the amount to be unvested - get the percentage for the iteration
    system_index system_table(get_self(), get_self().value);
    auto system_iterator = system_table.begin();
    check(system_iterator != system_table.end(), "system record not found");
    uint32_t unlock_percent = system_iterator->unlockpercent;

    // check that the unvest percentage is within limits
    check(unlock_percent > 0 && unlock_percent <= 100,
          "locked POINTs cannot be unlocked in this claim period. Please try during next claim period");


    asset locked_balance = asset(0, POINT_CURRENCY_SYMBOL);
    lockaccounts locked_accounts_table(get_self(), user.value);
    auto locked_account_iterator = locked_accounts_table.begin();

    if (locked_account_iterator != locked_accounts_table.end()) {
      locked_balance = locked_account_iterator->balance;
    }

    // if user's locked balance is 0 then nothing to do, so return
    if (locked_balance.amount == 0) {
      return;
    }

    // calculate the amount of locked POINTs to convert to liquid POINTs
    // Warning: these calculations use mixed-type arithmetic. Any changes need to
    // be thoroughly tested.

    double percentage = unlock_percent / 100.0;
    double locked_amount = locked_balance.amount / 10000.0;
    double percentage_applied = locked_amount * percentage;
    double adjusted_amount = ceil(percentage_applied); // rounding up to whole units
    uint64_t adjusted_units = adjusted_amount * 10000;

    // to prevent rounding up to more than the locked point balance, apply this adjustment
    // this will bring the locked balance to zero
    if (adjusted_units > locked_balance.amount) {
      adjusted_units = locked_balance.amount;
    }

    asset converted_points = asset(adjusted_units, POINT_CURRENCY_SYMBOL);

    string diagnostic =  "percentage: " + to_string(percentage) +
                        ", locked_amount: " + to_string(locked_amount) +
                        ", percentage_applied: " + to_string(percentage_applied) +
                        ", adjusted_amount: " + to_string(adjusted_amount) +
                        ", adjusted_units: " + to_string(adjusted_units) +
                        ", converted_points: " + converted_points.to_string();

    check(false, diagnostic);
  }

  if (action == "test has_nft") {
    bool nft_holder = has_nft(user);

    if (nft_holder) {
      check(false, "user has nft");
    } else {
      check(false, "user has no nft");
    }
  }

  if (action == "clear deposits") {
    deposits_index deposits_table(get_self(), get_self().value);
    auto deposit_iterator = deposits_table.begin();
    while (deposit_iterator != deposits_table.end()) {
      deposit_iterator = deposits_table.erase(deposit_iterator);
    }
  }

  if (action == "locked points") {
    symbol point_sym = symbol(POINT_CURRENCY_CODE, 4);
    lockaccounts locked_points_table(get_self(), user.value);
    auto locked_iterator = locked_points_table.find(point_sym.code().raw());

    if (locked_iterator == locked_points_table.end()) {
      locked_points_table.emplace(
        get_self(), [&](auto &l) {
          l.balance = asset(9000, POINT_CURRENCY_SYMBOL);
        });
    } else {
      locked_points_table.modify(locked_iterator, get_self(), [&](auto &l) {
        l.balance = asset(9000, POINT_CURRENCY_SYMBOL);
      });
    }
    
  }

  if (action == "constant test") {
    symbol ca_point_sym = symbol(POINT_CURRENCY_CODE, 4);
    asset ca = asset(1234567, ca_point_sym);

    symbol st_point_sym = symbol(POINT_CURRENCY_CODE, 4);
    asset st = asset(1234567, st_point_sym);

    check(false, "ca_asset = " + ca.to_string() + ", st asset = " + st.to_string());
  }
  
  if (action == "liquid points") {
    symbol point_sym = symbol(POINT_CURRENCY_CODE, 4);
    accounts points_table(get_self(), user.value);
    auto points_iterator = points_table.find(point_sym.code().raw());

    if (points_iterator == points_table.end()) {
      points_table.emplace(
        get_self(), [&](auto &l) {
          l.balance = asset(1000000000, POINT_CURRENCY_SYMBOL);
        });
    } else {
      points_table.modify(points_iterator, get_self(), [&](auto &l) {
        l.balance = asset(1000000000, POINT_CURRENCY_SYMBOL);
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


  if (action == "freebi balance") {
    asset freebi_balance;

    int64_t freebi_balance_amount = 0;  // default value
    string freebi_tokens_contract = get_parameter(name("freebitokens"));
    freebi_accounts freebi_accounts_table(name(freebi_tokens_contract), user.value);
    auto freebi_iterator = freebi_accounts_table.begin();
    if (freebi_iterator != freebi_accounts_table.end()) {
      freebi_balance = freebi_iterator->balance;
      freebi_balance_amount = freebi_balance.amount;
    }

    check(false, "user = " + user.to_string() + ", contract = " + freebi_tokens_contract + ", freebi_balance = " + freebi_balance.to_string());
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
    string freebi_tokens_contract = get_parameter(name("freebitokens"));
    accounts freebi_table(name(freebi_tokens_contract), user.value);
    auto freebi_iterator = freebi_table.begin();

    if (freebi_iterator != freebi_table.end()) {
      status_msg = status_msg + freebi_iterator->balance.to_string() + ", ";
    } else {
      status_msg = status_msg + "None, ";
    }

    // FREEOS
    status_msg = status_msg + "\nFREEOS: ";
    string freeos_tokens_contract = get_parameter(name("freeostokens"));
    accounts freeos_table(name(freeos_tokens_contract), user.value);
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
    string freebi_tokens_contract = get_parameter(name("freebitokens"));
    accounts freebi_table(name(freebi_tokens_contract), get_self().value);
    auto freebi_iterator = freebi_table.begin();

    if (freebi_iterator != freebi_table.end()) {
      status_msg = status_msg + freebi_iterator->balance.to_string() + ", ";
    } else {
      status_msg = status_msg + "None, ";
    }

    // FREEOS
    status_msg = status_msg + "\nFREEOS: ";
    string freeos_tokens_contract = get_parameter(name("freebitokens"));
    accounts freeos_table(name(freeos_tokens_contract), user.value);
    auto freeos_iterator = freeos_table.begin();

    if (freeos_iterator != freeos_table.end()) {
      status_msg = status_msg + freeos_iterator->balance.to_string() + ", ";
    } else {
      status_msg = status_msg + "None, ";
    }

    // supply POINT
    status_msg = status_msg + "\nPOINT supply: ";
    symbol point_sym = symbol(POINT_CURRENCY_CODE, 4);
    stats point_stat_table(get_self(), point_sym.code().raw());
    auto point_stat_iterator = point_stat_table.find(point_sym.code().raw());
    
    if (point_stat_iterator != point_stat_table.end()) {
      status_msg = status_msg + point_stat_iterator->supply.to_string() + ", ";
    } else {
      status_msg = status_msg + "None, ";
    }

    // supply FREEBI
    status_msg = status_msg + "\nFREEBI supply: ";
    symbol freebi_sym = symbol(FREEBI_CURRENCY_CODE, 4);
    stats freebi_stat_table(name(freebi_tokens_contract), freebi_sym.code().raw());
    auto freebi_stat_iterator = freebi_stat_table.find(freebi_sym.code().raw());
    
    if (freebi_stat_iterator != freebi_stat_table.end()) {
      status_msg = status_msg + freebi_stat_iterator->supply.to_string() + ", ";
    } else {
      status_msg = status_msg + "None, ";
    }

    // supply FREEOS
    status_msg = status_msg + "\nFREEOS supply: ";
    symbol freeos_sym = symbol(FREEOS_CURRENCY_CODE, 4);
    stats freeos_stat_table(name(freeos_tokens_contract), freeos_sym.code().raw());
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

    symbol xpr_sym = symbol(XPR_CURRENCY_CODE, 4);

    credit_index credit_table(get_self(), user.value);
    auto credit_iterator = credit_table.find(xpr_sym.code().raw());

    check(credit_iterator != credit_table.end(), "credit record not found");
    asset credit = credit_iterator->balance;
    string credit_msg = "credit for " + user.to_string() + " = " + credit.to_string();
    check(false, credit_msg);
  }

  if (action == "user credit FREEBI") {

    symbol xpr_sym = symbol(FREEBI_CURRENCY_CODE, 4);

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
      string freebi_tokens_contract = get_parameter(name("freebitokens"));
      token_contract = name(freebi_tokens_contract);
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