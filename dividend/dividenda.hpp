// "Ver 139, 21 May, 2021";
#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include <eosio/time.hpp>
#include <math.h>
#include "freeos.hpp"       
#include "dividend.hpp"     // Must be dividend.hpp - not dividenda :)

using namespace std;
using namespace eosio; 

/*!
 *@defgroup dividenda dividenda contract 
 *@ingroup eosiocontracts 
 *
 * dividenda contract
 *
 *@details dividenda contract read profit cumulated on contract account coming from different sources.
 *         
 *@{ 
 */

CONTRACT dividenda : public contract {

  // current version string:
  const std::string VERSION = "1.46";


  public:

    using contract::contract;

    /**
     * upsert action (updated) 
     *
     * @details Creates VIP table defining proposer and voters. After filling up the table may be changed only as a whole table. Individual 
     *          changes of rows are not allowed. This is assumed multisig to create the table.
     * 
     * Qparam[in] role_type - may be only: 1 for proposer, 2 for voter, 3 for another voter, more not accepted. 
     *                        Entering the same number twice is blocked.
     *            role_acct - corresponding account name of proposer or voters (depending on role type). 
     *                        This will be used to verify permissions
     *                        on proposal operations performed by another actions.
     * 
     * @brief The table 'white_list' is used by the action to define the context for the proposal creation and voting (Its defines
     *        who is the proposer and the two voters along with storing temporalily vote(s) casted by a given voter 
     *        on the current proposal (used only by the action 'proposalvote'). 
     * 
     * @pre contract permission is required            Note: This action has no entry from the frontend.
     *
     */
    [[eosio::action]]
    void upsert( uint8_t role_type, name role_acct ); 

    /**
     * remove action
     *
     * @details Removes row from the white_list table pointed by the parameter. This allows to enter a new row by the insert action
     *          with changed value..
     *
     * @param[in] role_acct - record with this account name will be removed. 
     *
     * @pre contract permission is required            Note: This action has no entry from the frontend.
     *   
     */
    [[eosio::action]]
    void remove( name role_acct );                      


    /**
     * dryrun action
     *
     * @details This action perform analytics related to the dividend percentage only. The action is showing what is  
     * the total percentage of dividends paid in each iteration (it will be the same for each) with breakdown into 
     * categories (like iterative, horizontal, and vertical roi caps). 
     * Additionally, it showing cumulative percentage of dividends paid for each person as each person may have several nftxs.
     * 
     * All of this is counted on a basis of all active at this moment nft's. nft's with rates paid already paid
     * (cap=1) or nft's completely paid due to total accrued above threshold value (cap=2) are not 
     * counted. Cap=3 founders nft's which are temporarily locked are counted as soon they will be paid again.   
     * Additional information on list of already fully paid nft's is displayed by the front-end.
     *
     * @param[in] proposername - account name of the proposer
     *
     * @param[out] (first name used by the bloks.io and eosstudio, second name used in the code)
     * -      ewstable table - displays cummulative percentage of active nft's on a basis of cap category
     * -      dryruns  table - displays cumulative percentage of active nft's on a basis of each user nft's.   
     *
     * @pre required any VIP account listed on whitelist.  
     *          
     */
    [[eosio::action]]
    void dryrun( name proposername ); 
   
    /*!
     * proposalnew action (proposal new)
     *
     * @details Creates new proposal. The proposal record (if accepted) becomes a base to mint new nft.
     * @param proposer          - The account name of a person eligible to create new proposal.
     * @param eosaccount        - The account name of future owner of new proposed nft (investor or founder).  
     * @param roi_target_cap    - Type of Dividend Policy (cap roi):  1- 2- 3-  (above 3 not accepted).
     * @param period_percentage - Pre-agreed percentage of weekly dividend which will be transfered to eosaccount.
     * @param threshold         - Upper limit for dividend payment (   if roi=1 threshold ignored, 
     *                                                                 if roi=2 threshold cummulative, 
     *                                                                 if roi=3 threshold for iteration).
     * @param rates_left        - Number of iteration dividend left to pay before this nft becomes inactive.
     *                            (used only for roi=1, for roi=2 and roi=3 ignored - enter 0 or above). 
     * @param locked            - Lock of payments (only for roi=3).
     *
     * @pre requires proposer permission 
     */
    [[eosio::action]]
    void proposalnew( const name proposername, const name eosaccount, const uint8_t roi_target_cap,      
        const double  period_percentage, const asset threshold, const uint32_t rates_left,        
        const bool locked ); 


    /**
     * proposalvote action.
     *
     * @details Voting for acceptance or not) the new proposal.  
     * @param votername - The voter name (must be white_list listed account with role_type 2 or 3, proposer not allowed).
     * @param vote  - The vote: 0 - ignored/no vote, 1 - no, 2 - yes/accept.
     * @brief Note: Second vote (if also positive) will mint the new proposed nft. The proposal in that case will 
     * be erased with writing "lastaccepted" in the proposal eosaccount field.  
     * Before writing new nft the function replay() is called to process remaining iterations
     * If proposal was not accepted is also 
     * erased with writing "lastrefused" in the proposal eosaccount field.  
     * There is not allowed (and verified) second voting for the same proposer.
     *
     * @pre requires voter permission
     */
    [[eosio::action]]
    void proposalvote( const name votername, const uint8_t vote );

    /**
     * propreset action.
     *
     * @details Erase and withdraw active proposal 
     * @brief The erased proposal will have written in the field eosaccount the text "erased". The nftkey in proposal 
     * record remains unchanged.
     *
     * @pre requires proposer permission
     */
    [[eosio::action]]
    void propreset( name proposername );  


    /**
     * removewhite action
     *
     * @details Removes the white_list table.
     * @brief This action should be considered to run only in testing or in case of administrative
     * changes in SDAO. 
     *
     * @pre requires contract permission
     */
    [[eosio::action]]
    void removewhite();                            


    /**
     * regchown action.
     *
     * @details Changes nft ownership (nft transfer).
     * Set number 2 in message table if nftx_id is not pointing nft belonging to userfrom. In that case the 
     * action regchown is ignored.
     *
     * @param userfrom - original nft owner
     * @param userto   - receiver of the nft
     * @param nftx_id  - the nft key belonging to userfrom
     * 
     * @pre requires userfrom permission.
     */
    [[eosio::action]] 
    void regchown(name userfrom, name userto, uint64_t nft_id); 


    /**
     * replay function
     *
     * @details process dividend sharing for each iteration one by one except the current which is still collecting incoming profits. 
     * Set up number 1 in the message table if received current iteration number is 0, what is unlikely to happen.
     *
     * @pre do not need any
     */
    void replay();

   /**
    * dividcompute action
    * 
    * @details The wrapper for the replay() function allowing to call it as an action.
    *
    * @brief 
    *
    */
    [[eosio::action]]
    void dividcompute();


    /**
    * cron action
    * 
    * @details Provides an interface to the Proton CRON service. The action invokes dividcompute.
    *
    * @brief 
    *
    */
    [[eosio::action]]
    void cron();
          
    
                         

    /**
     * unlocknftx action      
     *
     * @param nftx_id - points nftx which will have removed lock. It must be founder owned nftx otherwise action is ignored.
     * 
     * @details Removes lock (if any) on nft belonging to a founder. If applied to alredy unlocked nft, the action is ignored.
     * In that case the number 3 is set in the message table.
     * @pre requires contract authorization 
     * 
     */
    [[eosio::action]]
    void unlocknft( uint64_t nft_id );


    /**
    * query action
    * (informative only - the action do not change any values) 
    *
    * @param eosaccount - the account which we query. This account is returned from frontend wallet as an account
    * of a person who currently is logged on. 
    * The function return the type of the account to notify_front where is used immediately by the front end 
    * to display a correct web page.
    *
    * @pre permissions are the same like eosaccount parameter
    *
    */
    [[eosio::action]]
    void query( name eosaccount ); 


    // MAINTAIN
    [[eosio::action]]
    void maintain( string action, name user ); 

              
  
    // helper functions used only internally 

    /**
     * notify_front function
     * @details Used to transmit non-critical error messages and warnings to be interpreted by the front end.
     * @param number - one of the numeric const defined in dividend.hpp. Each time when notify_front is called the  
     * given number is added to the queue (a vector). The vector is read by the front end.
     * The vector can be erased by the clearfront function called when new proposal is created. 
     */
    void notify_front( uint8_t number );     


    /**
     * clearfront function
     * @details Clearing content of notify_front.
     * Called when new proposal is created.
     */
    void clearfront();  
 
  
   /**
    * action version
    * @details breaks the action and display string with version number + current iteration number
    */ 
    [[eosio::action]] 
    void version() {
      uint32_t this_iteration = getclaimiteration();

      std::string version_message = "Version: " + VERSION + " - iteration " + std::to_string(this_iteration);
      check(false, version_message);
    }

  // P R I V A T E //

  private:

    static inline time_point_sec current_time_point_sec() {
      return time_point_sec(current_time_point());
    }

    uint32_t now() {
      return current_time_point().sec_since_epoch();
    }

    // time_point_sec(current_time_point().sec_since_epoch());  // now()

    void dividendhand( uint64_t periodpoint, asset profit ); //This should be called only as internal function from the 
                                                             // other action due to security.
  
  
    
  TABLE status_messages {                 // Message numbers for the latest proposal - to be interpreted by the frontend. 
                                          // New proposal erase this list.
    uint64_t key;
    uint8_t  errorno;
    auto primary_key() const { return key; }
  };
  using messages_table = eosio::multi_index<"messages"_n, status_messages>;

  TABLE proposal_struct {                     
      uint64_t key;                       //!< primary key = always 1.
      name eosaccount;                    //!< POINT account used to receive dividends and for identification
      uint8_t roi_target_cap;             //!< =1 iterative cap, =2 horizontal, =3 vertical
      double proposal_percentage;         //!< iteration percentage (for each iteration is the same)
      bool locked;                        //!< lock dividends for selected new members. Note: When unlock cannot be locked again.
      uint32_t expires_at;                //!< expiry for the proposal only (must be voted before that time)
      asset threshold;                    //!< max total divident (2) for horizontal cap or max weekly dividend for vertical (3) cap 
      uint32_t rates_left;                //!< count down payments left in iteration cap (I) only  
      asset accrued;
      uint64_t primary_key() const { return key; }
  };
  // typedef eosio::multi_index<"proposals"_n, proposal_struct> proposal_table;
  using proposal_table = eosio::multi_index<"proposals"_n, proposal_struct>;

  TABLE nft_struct {                        //!< Each record is a single NFT by itself
      uint64_t nft_key;
      name     eosaccount;                  //!< POINT account used to receive dividends and for identification (as a secondary key)
      uint8_t  roi_target_cap;              //!< 1- iterative 2- horizontal 3- vertical 
      double   nft_percentage;              //!< Only this is used for counting dividend to pay - the other parameters examine eligibility,
      time_point_sec mint_date;             //!< NFT mint date. In fact, the current date of the moment when this nftx record was created,
      bool     locked;                      //!< lock dividends for selected new members. Note: When unlock should be not lock again.
      asset    threshold = asset(0,symbol("POINT",4) ); //!< max total divident (2) for horizontal cap or max weekly dividend for vertical (3) cap
      uint32_t rates_left;                  //!< count down payments left in iteration cap=1 only   
      asset    accrued = asset(0,symbol("POINT",4) ); ;                       
      uint64_t primary_key() const {return nft_key;}
  };
  using nft_table = eosio::multi_index<"nfts"_n, nft_struct>; 

  // MAINTAIN - COPYNFTS
  TABLE copynft_struct {                        //!< Each record is a single NFT by itself
      uint64_t nft_key;
      name     eosaccount;                  //!< POINT account used to receive dividends and for identification (as a secondary key)
      uint8_t  roi_target_cap;              //!< 1- iterative 2- horizontal 3- vertical 
      double   nft_percentage;              //!< Only this is used for counting dividend to pay - the other parameters examine eligibility,
      time_point_sec mint_date;             //!< NFT mint date. In fact, the current date of the moment when this nftx record was created,
      bool     locked;                      //!< lock dividends for selected new members. Note: When unlock should be not lock again.
      asset    threshold = asset(0,symbol("POINT",4) ); //!< max total divident (2) for horizontal cap or max weekly dividend for vertical (3) cap
      uint32_t rates_left;                  //!< count down payments left in iteration cap=1 only   
      asset    accrued = asset(0,symbol("POINT",4) ); ;                       
      uint64_t primary_key() const {return nft_key;}
      uint64_t get_secondary() const { return eosaccount.value; }
  };
  using copynft_table = eosio::multi_index<"copynfts"_n, copynft_struct, indexed_by<"account"_n, const_mem_fun<copynft_struct, uint64_t, &copynft_struct::get_secondary>>>;


  TABLE whitelist_struct {                 //!< whitelist_struct - List of allowed proposers and voters along with their vote.
      uint64_t idno;                       //!< (1-proposer, 2,3 voters)
      name     user;                       
      uint8_t  vote;                       //!< 0 -n/a  1 - no   2 - yes /     - different than zero if already voted
      uint64_t primary_key() const { return user.value; }   //!< This table should be filled up only by a real multisig
      uint64_t get_secondary_1() const { return idno; }
  };
  using whitelist_index = eosio::multi_index<"whitelist"_n, whitelist_struct,
    indexed_by<"byidno"_n, const_mem_fun<whitelist_struct, uint64_t, &whitelist_struct::get_secondary_1>>>;


  TABLE recive_struct {
      name     user;
      asset    to_receive;  //!<This is a cumulative amount of POINT tokens which will be paid to the user on a basis of all owned nfts. 
      uint64_t primary_key() const {return user.value; }
  };
  using recive_index = eosio::multi_index<"recives"_n, recive_struct>;  

  
  TABLE dryrun_struct { //!< dryrun table to keep statistics by user
    name user;
    double byusertotal;   //!< Total percentage value of all active NFTs for a given user.
    uint64_t primary_key() const {return user.value; }
  };
  using dryrun_data = eosio::multi_index<"dryruns"_n, dryrun_struct>;

// imported declaration 

// iteration table definition - updated 5th May 21.
// freeos airclaim iteration calendar - code: freeosconfig, scope: freeosconfig
        struct [[eosio::table]] iteration {
          uint32_t    iteration_number;
          time_point  start;
          time_point  end;
          uint16_t    claim_amount;
          uint16_t    tokens_required;

          uint64_t primary_key() const { return iteration_number; }
          uint64_t get_secondary() const {return start.time_since_epoch()._count;}
        };

        // using iteration_index = eosio::multi_index<"iterations"_n, iteration>;

        using iterations_index = eosio::multi_index<"iterations"_n, iteration,
        indexed_by<"start"_n, const_mem_fun<iteration, uint64_t, &iteration::get_secondary>>
        >;

 //--- freedao deposits table ---//
  struct [[eosio::table]] deposit {  //
      uint64_t   iteration;
      asset      accrued;
      uint64_t primary_key()const { return iteration; }
  };
  using deposit_index = eosio::multi_index<"deposits"_n, deposit>;
  // end of imported -----------------------------------------------------

  struct [[eosio::table]] ewstable { //!< dryrun table to keep value of all active NFT's by their categories.
     uint64_t category;
     double bycategory;
     uint64_t primary_key() const { return category; }
  };
  // typedef eosio::multi_index< "ewstables"_n, ewstable > ewsanalytics; 
  using ewsanalytics = eosio::multi_index<"ewstables"_n, ewstable>;


// returns the current iteration number  Updated 5th May 21.
uint32_t getclaimiteration() {

  uint32_t this_iteration = 0;

  uint64_t now = current_time_point().time_since_epoch()._count;

  // iterate through iteration records and find one that matches current time
  iterations_index iterations_table(name(freeosconfig_acct), name(freeosconfig_acct).value);
  auto start_index = iterations_table.get_index<"start"_n>();
  auto iteration_record = start_index.upper_bound(now);

  if (iteration_record != start_index.begin()) {
    iteration_record--;
  }

  // check we are within the period of the iteration
  if (iteration_record != start_index.end() && now >= iteration_record->start.time_since_epoch()._count
      && now <= iteration_record->end.time_since_epoch()._count) {
    this_iteration = iteration_record->iteration_number;
  }  
 
  return this_iteration;
}


/**
 * auth_vip function
 *
 * @details Proposer and voters verification function
 * @param[in] user - name of user to search in whitelist table
 * @param[out] auth_vip - number 0 if not, 1 - for proposer, 2 or 3 for voters.
 *
 */
uint8_t auth_vip( name user ){
  whitelist_index white_list(get_self(), get_self().value); 
  auto v = white_list.find(user.value); // Is the user on the list?
  if (v!=white_list.end()){ // process
    return v->idno;     // user verified 1,2,  or 3.
  } 
  return 0;  // user not verified 0.   
}

void upsert_value( name user, asset to_receive ){ // This replaces identical pieces of code
  //Add 'to_receive' to 'receivers_list' for current user.   

  recive_index receivers_list( get_self(), get_self().value ); ///< Rows summs payments to receive, by each user, from owned NFTs.
  auto idx = receivers_list.find(user.value); 
  if( idx == receivers_list.end() ) // first NFT for a given user processed
  {
    receivers_list.emplace( get_self(), [&]( auto& row ){
    row.user = user;
    row.to_receive = asset(0,symbol("POINT",4) ); // typecast for asset
    row.to_receive = to_receive;
    });
  }
  else // not first NFT processed for a given user - 
       // add value of current NFT to already owned/processed by the user
  { 
    receivers_list.modify(idx, get_self(), [&]( auto& row ) {  
    row.to_receive += to_receive;           
    }); 
  } 
} //end


void upsert_perc(uint8_t cap_type, name user, double nft_percentage ) //This replaces identical pieces of code in dryrun.
{                                                  // RATIONALE
  
  dryrun_data  list_by_user( get_self(), get_self().value );
  ewsanalytics analytics(get_self(), get_self().value );  
  auto idx = list_by_user.find(user.value);    // idx - pointing user in 'list_by_user' for current NFT
  auto idy = analytics.find( cap_type );       // idy = pointing category in 'analytics' for current NFT

  analytics.modify(idy, get_self(), [&](auto& rr){ // I know how many categories I have so I have
  rr.bycategory += nft_percentage;                 // created an empty table with proper size.
  });

  if( idx ==  list_by_user.end() )                 // I do not know how many and which users I will have
  {                                                // so I need to add each new user which appeared when
    list_by_user.emplace( get_self(), [&]( auto& row )  // processing sequentially nft_register.
    {
      row.user = user;
      row.byusertotal = nft_percentage;
    });
  }
  else 
  {
    list_by_user.modify(idx, get_self(), [&]( auto& row ) {
    row.byusertotal += nft_percentage;
    });
  };
}



    
};
/** @}*/ // end of @defgroup dividenda freeosdivide contract


        
