
using namespace eosio;
using namespace std;

// Versions description and CHANGE LOG
 
// 130 - identification function auth_vip to identify members of whitelist table. Changes in names. 20th Apr.
// 131 - added upsert_value and upsert_perc also two 
//       secondary index search verification 
// 132 - optionsdiv freeos freeosconfig allocate 28 04 2021 Changed external naming convention
// 133 - detour around 'replay' allowing minting NFT when iteration zero
// 134 - the same source code is used since now for all the environments - only dividend.hpp has different external file references for all environments
// 135 - adding remove record for whitelist table
// 136 - adding logs table and daoaccount divided counted as asset not percentage
// 137 - minor updates for the logs table
// 138 - further improvements for logs/ Added lock verification on proposal entry parameters.

constexpr static uint64_t EXPIRATION_PERIOD = 60 * 60;  ///< default time limit for NFT proposal voting - one hour in seconds. 
// const static double ACC_MARGIN =  0.0;  // margin - % of 'dust' left on freeosdivide account for freeosdaodao transaction. 
const static double ALLOWED_PERCENTAGE = 90.00; ///< cumulative percentage of all active NFT's. Above this no more proposals allowed.

enum role_type { PROPOSER=1, VOTER1=2, VOTER2=3}; 
enum roi_target_cap { ITERATION=1, HORIZONTAL=2, VERTICAL=3 };

// Accounts names
const name freeos_acct  = "freeosgov2"_n;   // freeos governance contract
const name daoaccount   = "freeosdaodao"_n; // target DAO account


//                          Non-catastrophic frontend warnings used in 'messages' (notify_front)
//                          Pre-defined messages to be interpreted by the frontend.

const int UNKNOWN_ITERATION = 1; // '1 - Current iteration number received from airgrab is zero!'       red notification
const int ACTION_IGNORED    = 2; // '2 - Action ignored: nftx_key pointing nft not owned by userfrom'   yellow
const int NON_FOUNDER       = 3; // '3 - Action ignored: trying to unlock non-founder account'          yellow 
                                 // Note: Unlocking already unlocked founder account is ignored without a message. 
const int NFT_MINTED        = 4; // '4 - Info: proposal accepted and saved. nft minted'                 green 
const int NFT_REFUSED       = 5; // '5 - Action ignored: proposal not accepted in voting'               yellow
const int NFT_CHOWN         = 6; // '6 - ONFT ownership change'                                         green
const int ITERATION_ZERO    = 7; // '7 - warning on iteration zero when mint NFT                        yellow 


