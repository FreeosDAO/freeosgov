//#pragma once
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include "freeosgov.hpp"
#include "tables.hpp"
#include "constants.hpp"

using namespace eosio;
using namespace freedao;
using namespace std;

/** @defgroup claim Claiming
 *  These Actions and functions are related to the weekly claim of each participant.
 *  @{
 */


/**
 * Action checks that the user has completed the required number of surveys, votes and ratifications, and
 * if so, it mints the appropriate number of POINTs and transfers them to the user's account
 * 
 * @param user the account name of the user who is claiming
 */
void freeosgov::claim(name user) {

    require_auth(user);

    // check that system is operational (masterswitch parameter set to "1")
    check(check_master_switch(), MSG_FREEOS_SYSTEM_NOT_AVAILABLE);

    tick();

    uint32_t this_iteration = current_iteration();
    
    // is the system operational?
    check(this_iteration != 0, "The freeos system is not available at this time");

    // find the participant's last claim
    participants_index participants_table(get_self(), user.value);
    auto participant_iterator = participants_table.begin();
    check(participant_iterator != participants_table.end(), "user registration record is undefined");
    uint32_t last_claim = participant_iterator->last_claim;

    // find the user's svr record
    svr_index svr_table(get_self(), user.value);
    auto svr_iterator = svr_table.begin();
    check(svr_iterator != svr_table.end(), "user has not completed any votes or surveys");

    // check if user has already claimed in this iteration
    check(this_iteration != last_claim, "you cannot claim more than once in an iteration");

    // check that we are not in iteration 1 - payments can only be made retrospectively
    check(this_iteration > 1, "claims are for previous iterations only");

    // get the current proportion of locked POINTs
    double locked_proportion = get_locked_proportion();

    // determine which range of iterations we are paying for
    uint32_t earliest_payment_iteration = this_iteration > 4 ? this_iteration - 4 : 1;
    uint32_t latest_payment_iteration = this_iteration - 1;

    // open the rewards table
    rewards_index rewards_table(get_self(), get_self().value);

    // get the S,V,R share values
    double survey_share = get_dparameter(name("surveyshare"));
    double vote_share = get_dparameter(name("voteshare"));
    double ratify_share = get_dparameter(name("ratifyshare"));

    double freedaoshare = get_dparameter(name("freedaoshare"));
    double partnershare = get_dparameter(name("partnershare"));

    // get the freeosdiv and partners accounts
    name freeosdiv_acct = name(get_parameter(name("freedaoacct")));
    name partners_acct = name(get_parameter(name("partnersacct")));

    // keep some counters
    asset   total_user_payment = asset(0, POINT_CURRENCY_SYMBOL);
    asset   grand_total_iterations_payments = asset(0, POINT_CURRENCY_SYMBOL);
    uint8_t total_issuances = 0;


    // determine each iteration's payments
    for (uint32_t iter = earliest_payment_iteration; iter <= latest_payment_iteration; iter++) {
        
        // has the user already claimed for this iteration?
        // developer note: this next statement could be incorporated into the for loop, but implementing this way for clarity
        if (iter <= last_claim) continue;

        // get the reward record for the iteration
        auto reward_iterator = rewards_table.find(iter);
        if (reward_iterator == rewards_table.end()) continue;   // no reward record so continue to next iteration

        // check if the vote was ratified, otherwise no payment to be made for that iteration
        if (reward_iterator->ratified == false) continue;

        // running total of user payments amounts
        uint64_t user_payment_amount = 0;

        // initialise the memo
        string transfer_memo = "Claim for week " + to_string(iter) + ": ";

        // determine the total user payout for the iteration
        uint64_t iteration_reward_amount = reward_iterator->participant_issuance.amount;
        
        // split the total user payout according to whether S, V or R completed
        uint64_t payment_survey_amount = iteration_reward_amount * survey_share;
        uint64_t payment_vote_amount = iteration_reward_amount * vote_share;
        uint64_t payment_ratify_amount = iteration_reward_amount * ratify_share;

        // check if survey completed for this iteration
        transfer_memo += "Survey [" + to_string((int)(survey_share * 100)) + "%]: ";
        if (svr_iterator->survey0 == iter ||
            svr_iterator->survey1 == iter ||
            svr_iterator->survey2 == iter ||
            svr_iterator->survey3 == iter ||
            svr_iterator->survey4 == iter) {
                user_payment_amount += payment_survey_amount;
                transfer_memo += "(" + asset(payment_survey_amount, POINT_CURRENCY_SYMBOL).to_string() + ")";
            } else {
                transfer_memo += "(not completed)";
            }

        // check if vote completed for this iteration
        transfer_memo += ", Vote [" + to_string((int)(vote_share * 100)) + "%]: ";
        if (svr_iterator->vote0 == iter ||
            svr_iterator->vote1 == iter ||
            svr_iterator->vote2 == iter ||
            svr_iterator->vote3 == iter ||
            svr_iterator->vote4 == iter) {
                user_payment_amount += payment_vote_amount;
                transfer_memo += "(" + asset(payment_vote_amount, POINT_CURRENCY_SYMBOL).to_string() + ")";
            } else {
                transfer_memo += "(not completed)";
            }

        // check if ratify completed for this iteration
        transfer_memo += ", Ratify [" + to_string((int)(ratify_share * 100)) + "%]: ";
        if (svr_iterator->ratify0 == iter ||
            svr_iterator->ratify1 == iter ||
            svr_iterator->ratify2 == iter ||
            svr_iterator->ratify3 == iter ||
            svr_iterator->ratify4 == iter) {
                user_payment_amount += payment_ratify_amount;
                transfer_memo += "(" + asset(payment_ratify_amount, POINT_CURRENCY_SYMBOL).to_string() + ")";
            } else {
                transfer_memo += "(not completed)";
            }
        
        // make the payment, if any
        if (user_payment_amount > 0) {
            total_issuances++;

            uint64_t user_locked_amount = user_payment_amount * locked_proportion;
            uint64_t user_liquid_amount = user_payment_amount - user_locked_amount;

            // get the partner and freedao shares
            uint64_t freedao_payment_amount = user_payment_amount * freedaoshare;
            uint64_t partners_payment_amount = user_payment_amount * partnershare;

            // calculate the payments as assets
            asset user_payment = asset(user_payment_amount, POINT_CURRENCY_SYMBOL);
            asset user_liquid_payment = asset(user_liquid_amount, POINT_CURRENCY_SYMBOL);
            asset user_locked_payment = asset(user_locked_amount, POINT_CURRENCY_SYMBOL);
            asset freedao_payment = asset(freedao_payment_amount, POINT_CURRENCY_SYMBOL);
            asset partners_payment = asset(partners_payment_amount, POINT_CURRENCY_SYMBOL);

            total_user_payment += user_payment;

            // mint the total amount
            asset iteration_all_payments = user_payment + freedao_payment + partners_payment;
            grand_total_iterations_payments += iteration_all_payments;

            string issue_memo = "claim by " + user.to_string() + " for week " + to_string(iter);
            
            // issue all POINTs
            action issue_action = action(
                permission_level{get_self(), "active"_n}, get_self(),
                "mint"_n, std::make_tuple(get_self(), get_self(), iteration_all_payments, issue_memo));
            issue_action.send();

            // explain the unlocked/locked amounts in the memo
            transfer_memo += " - payment (unlocked/locked) = " + user_liquid_payment.to_string() + "/" + user_locked_payment.to_string();

            // transfer liquid POINTs to the user account
            action user_transfer_action = action(
            permission_level{get_self(), "active"_n}, get_self(),
                "allocate"_n,
                std::make_tuple(get_self(), user, user_liquid_payment, transfer_memo));
            user_transfer_action.send();

            // increase the user's locked balance
            if (user_locked_amount > 0) {
                lockaccounts lock_accounts(get_self(), user.value);
                auto acct_iterator = lock_accounts.find(user_locked_payment.symbol.code().raw());
                if (acct_iterator == lock_accounts.end()) {
                    lock_accounts.emplace(get_self(), [&](auto &a) { a.balance = user_locked_payment; });
                } else {
                    lock_accounts.modify(acct_iterator, get_self(), [&](auto &a) { a.balance += user_locked_payment; });
                }
            }

            // memo for freedao and partners shares
            string shares_memo = string("share of claim by ") + user.to_string() + " for week " + to_string(iter);

            // transfer POINTs to the freedao account
            // first, record the deposit to the freedao account
            record_deposit(this_iteration, freedao_payment);

            if (freedao_payment_amount > 0) {
                action freedao_transfer_action = action(
                permission_level{get_self(), "active"_n}, get_self(),
                    "allocate"_n,
                    std::make_tuple(get_self(), freeosdiv_acct, freedao_payment, shares_memo));
                freedao_transfer_action.send();
            }
            
            // transfer POINTs to the partners account
            if (partners_payment_amount > 0) {
                action partners_transfer_action = action(
                permission_level{get_self(), "active"_n}, get_self(),
                    "allocate"_n,
                    std::make_tuple(get_self(), partners_acct, partners_payment, shares_memo));
                partners_transfer_action.send();
            }
        }
    }

    // update the participant record issuance values
    participants_table.modify(participant_iterator, get_self(), [&](auto &participant) {
        participant.last_claim = latest_payment_iteration;   // i.e. paid up to this iteration
        participant.total_issuance_amount += total_user_payment;
        participant.issuances += total_issuances;
    });
    

    // update the number of claimevents in the system record and reduce CLS
    if (total_issuances > 0) {
        system_index system_table(get_self(), get_self().value);
        auto system_iterator = system_table.begin();
        check(system_iterator != system_table.end(), "system record is undefined");
        system_table.modify(system_iterator, get_self(), [&](auto &s) {
            s.claimevents += total_issuances;
            s.cls -= grand_total_iterations_payments;   // reduce CLS by total number of points issued
        });
    }
    
}

std::string trim(const std::string& str) {
    // Trim leading whitespace
    auto start = std::find_if_not(str.begin(), str.end(), [](unsigned char ch) {
        return std::isspace(ch);
    });
    
    // Trim trailing whitespace
    auto end = std::find_if_not(str.rbegin(), str.rend(), [](unsigned char ch) {
        return std::isspace(ch);
    }).base();
    
    // Return the trimmed string
    return (start < end ? std::string(start, end) : std::string());
}

/**
 * Notification function enables the user to burn a specified amount of FREEOS.
 * These tokens are burned, and an entry is made in the swaps table to record the transaction.
 * The Internet Computer side of the transaction reads the swaps table entries and mints the equivalent
 * number of (IC-based) FREEOS.
 * 
 * @param from - the account sending the tokens,
 * @param to - the 'freeos' account receiving the tokens,
 * @param quantity - the asset, i.e. an amount of FREEOS which is transferred to the freeosgov account,
 * @param memo - a memo describing the transaction - must have the correct memo to initiate the swap process
 */
[[eosio::on_notify("freeostokens::transfer")]] void freeosgov::ic_swap(name from, name to, asset quantity, std::string memo) {

    std::string required_memo_prefix = "IC SWAP ";
    std::string ic_principal = "";

    // Check if the memo starts with the prefix "IC SWAP "
    if (memo.find(required_memo_prefix) == 0) {
        // Extract the principal after "IC SWAP "
        ic_principal = trim(memo.substr(required_memo_prefix.length()));
    } else {
        // invalid memo format - so this is not a swap
        return;
    }

    uint32_t icswapopen = get_iparameter(name("icswapopen"));
    uint32_t icswapclose = get_iparameter(name("icswapclose"));
    uint32_t currenttime = current_time_point().sec_since_epoch();
    check(currenttime >= icswapopen && currenttime <= icswapclose, "The IC swap facility is not available at this time");


    check( quantity.symbol.code().to_string() == FREEOS_CURRENCY_CODE, "The quantity must be in " + FREEOS_CURRENCY_CODE + " tokens, e.g. 123.0000 " + FREEOS_CURRENCY_CODE );
    
    // check if user has reached their swap allowance
    asset user_swap_total = asset(0, FREEOS_CURRENCY_SYMBOL);   // default value
    swaptotals_index swaptotals_table(get_self(), from.value);
    auto swaptotals_iterator = swaptotals_table.begin();
    if (swaptotals_iterator != swaptotals_table.end()) {
        user_swap_total = swaptotals_iterator->total;
    }

    uint32_t icswaplimit = get_iparameter(name("icswaplimit"));
    asset swaplimit = asset(icswaplimit * FREEOS_UNIT_MULTIPLIER, FREEOS_CURRENCY_SYMBOL);

    check((user_swap_total.amount + quantity.amount) <= swaplimit.amount,
     "The swap quantity will exceed the swap allowance (" + user_swap_total.to_string() +
     " already swapped out of an allowance of " + swaplimit.to_string() + ")"
     );

    check(ic_principal.length() > 0, "The transfer memo must include the user's IC principal, e.g. 'IC SWAP w7x3r-cok77-xa'");

    string retire_memo_str = "Swap " + quantity.to_string() + " from " + from.to_string() + " to IC principal " + ic_principal;

    // Burn the tokens
    action(
        permission_level{get_self(), "active"_n},
        name("freeostokens"),
        name("retire"),
        std::make_tuple(quantity, retire_memo_str)
    ).send();

   // record this swap
   swaps_index swaps_table(get_self(), get_self().value);
   swaps_table.emplace(get_self(), [&](auto &s) {
      s.proton_account = from;
      s.ic_principal = ic_principal;
      s.amount = quantity;
      s.utc_time = current_time_point().sec_since_epoch();
    });

    // update the user's new swap total
    if (swaptotals_iterator == swaptotals_table.end()) {
        swaptotals_table.emplace(get_self(), [&](auto &t) { t.total = quantity; });
    } else {
        swaptotals_table.modify(swaptotals_iterator, get_self(), [&](auto &t) { t.total += quantity; });
    }
   
}

/** @} */ // end of claim group