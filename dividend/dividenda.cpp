#include "dividenda.hpp" 

using namespace freedao;

  /*
   +-----------------------------------
   +  upsert -  
   +-----------------------------------
             +
             +  - Creates whitelist table item by item.   
             */
[[eosio::action]]
void dividenda::upsert( uint8_t role_type, name role_acct )  //!< insert new item to white_list.
{  // The role types are enumerated key = 1 proposer key=2 voter key=3 second voter.
   require_auth( get_self() );
   check( (role_type==PROPOSER)||(role_type==VOTER1)||(role_type==VOTER2), "Not correct role type!");  
   whitelist_index white_list(get_self(), get_self().value);

   // PSEUDOCODE:
   // if (record_exists) then modify_it
   // else if 'less than three records' then emplace
   //      else 'nothing to do'. 

   auto ite = white_list.find( role_acct.value ); // Revoke this action! 
   if ( ite != white_list.end() ){ // Modify has no sense - what will be modified ???
        white_list.modify(ite, get_self(), [&](auto &p) {
          p.idno = role_type;
          p.user = role_acct; // Entered duplicated value cannot be verified!! //Rather send message that record must be removed first - no modify
          p.vote = 0;
        });
   } else {
       // It should be exactly three records. role_types and role_accts cannot repeat.
       uint8_t i=0;
       for( auto iter=white_list.begin(); iter!=white_list.end(); iter++ ){
          check(iter->idno!=role_type, "The supplied role type already exists!");
          check(iter->user!=role_acct, "The supplied role account already exists!"); 
          i++; // Count the records
       }
       notify_front( i ); //TEST 
       if(i<3){
          white_list.emplace(get_self(), [&]( auto& row )
          {
            row.idno = role_type;  
            row.user = role_acct; 
            row.vote = 0;         // Not voted yet. 
          });
       } else { check( false, "The whitelist table has already three items. Remove one if insert a new one.");
              }
     } 
}

  /*
   +-----------------------------------
   +  remove -  
   +-----------------------------------
             +
             +  - Remove one row from the white_list table.   
             */
[[eosio::action]]
void dividenda::remove( name role_acct )   //!< remove one item from white_list
{
  require_auth( get_self() );
  whitelist_index white_list(get_self(), get_self().value);
    auto ite = white_list.find( role_acct.value );
    check(ite != white_list.end(), "Record to remove does not exist.");
    white_list.erase(ite);
}
//
//---


  /*
   +-----------------------------------
   +  removewhite -  
   +-----------------------------------
             +
             +  Remove completely white_list table.   
             */

[[eosio::action]]
void dividenda::removewhite(){
  require_auth( _self );
  whitelist_index white_list(get_self(), get_self().value);
  // Delete all records in _messages table
  auto itr = white_list.begin();
    while (itr != white_list.end()) {
       itr = white_list.erase(itr);
    }
}
//
//---


  /*
   +----------------------------------------
   +  proposalnew - 
   +----------------------------------------
             +
             +     - Creating a new proposal
             */
[[eosio::action]]
void dividenda::proposalnew(
  		  const name      proposername,
        const name      eosaccount,          //!< POINT account used to receive dividends and for user identification
        const uint8_t   roi_target_cap,      // 1-(I)teration, 2-(H)orizontal, 3-(V)ertical
        const double    proposal_percentage,
        const asset     threshold,           // max value for total (cap=2) or weekly (cap=3) pay-cut actions
        const uint32_t  rates_left,          // number of dividend rates which cause pay-stop (roi_target_cap=1)
        const bool      locked               //!< lock dividends for selected new founders. Note: When unlock cannot be locked again.
    )
{     

	//Verify proposer against white_list but not against other authorization.
  require_auth(proposername);
	check( (auth_vip(proposername)==PROPOSER), "proposername not authorized by whitelist!" );
  
  clearfront(); // Remove past warnings before any processing

  // erase voting results!    v1.45
  whitelist_index white_list(get_self(), get_self().value);
  for( auto iter=white_list.begin(); iter!=white_list.end(); iter++ )
    {
      white_list.modify(iter, get_self(), [&]( auto& row )
      {
        row.vote = 0;
      }); 
    }
  // end of v1.45 change

  // Entry parameters verification:
  check( (roi_target_cap==ITERATION)||(roi_target_cap==HORIZONTAL)||(roi_target_cap==VERTICAL), "Wrong value of roi_target_cap!");  
  check( (proposal_percentage >0 && proposal_percentage <100),  "The proposal_percentage out of range!" );
  check( is_account( eosaccount ), "eosaccount does not exist!"); 
  auto sym = threshold.symbol; 
  auto obj = symbol( "POINT", 4);
  check( (sym==obj), "The symbol is not POINT!");
  if (roi_target_cap==ITERATION) check( (rates_left>0), "This is Iteration Cap(1), rates_left should be above zero!");
  if (roi_target_cap!=ITERATION) check( threshold.amount > 0, "Must enter positive threshold!" );
  if ( locked ) check( roi_target_cap==VERTICAL, "Only Vertical Cap can be locked!" );
 
  // summarize the analytics table (used data are coming from previous run or dryrun)
  ewsanalytics analytics(get_self(), get_self().value ); 
  double total_percentage = 0.0;  
  for (auto aitr = analytics.begin(); aitr != analytics.end(); aitr++)
  {
    total_percentage = total_percentage + aitr->bycategory; 
  }
  total_percentage = total_percentage + proposal_percentage; 
  check( (total_percentage <= ALLOWED_PERCENTAGE), "The proposal_percentage >= ALLOWED_PERCENTAGE. Make dryrun. Delay new proposal.");

  proposal_table proposal(get_self(), get_self().value);
  auto pro_itr = proposal.begin();
  if( pro_itr == proposal.end() )
 	{
  	 proposal.emplace(get_self(), [&](auto &p) { // This is only first time
        p.key                 = 1;
        p.eosaccount          = eosaccount; 
        p.roi_target_cap      = roi_target_cap;
        p.proposal_percentage = proposal_percentage;
        p.threshold           = threshold;
        p.rates_left          = rates_left;
        p.locked              = locked;
        p.accrued             = asset(0,symbol("POINT",4) ); 
        p.expires_at          = now()+EXPIRATION_PERIOD;
     });
	}
  else
 	{ // all the other times except first time is going this way:
    // we know here that proposal exists
    // we Process only if no active proposals. This prevents editing existing proposal.
    check( (pro_itr->expires_at < now() ), "There exists not expired proposal! Consider removing it.");
  	proposal.modify(pro_itr, get_self(), [&](auto &p) {
        p.key                 = 1;
        p.eosaccount          = eosaccount; 
        p.roi_target_cap      = roi_target_cap;
        p.proposal_percentage = proposal_percentage;
        p.threshold           = threshold;
        p.rates_left          = rates_left; 
        p.locked              = locked;
        p.accrued             = asset(0,symbol("POINT",4) ); 
        p.expires_at          = now()+EXPIRATION_PERIOD;
    });
  }    	
} 
//
//---


  /*
   +------------------------------------
   +  proposalvote -  
   +------------------------------------
             +
             +  - voting for proposal    
             */
[[eosio::action]]
void dividenda::proposalvote(  const name votername,
                               const uint8_t uservote    )
{       
    //---------------
    // Verify the voter who is voting right now: 
        require_auth(votername);

        // Verify completness 'white_list' table before any processing
        //   to not verify record existence later.
        whitelist_index white_list(get_self(), get_self().value);
        uint8_t i=0;
        for( auto iter=white_list.begin(); iter!=white_list.end(); iter++ ){
            i++; // Count the records
        }  
        check((i==3), "white_list table incomplete!" );  

        // Verify proposal correctness:
        proposal_table proposal(get_self(), get_self().value);
        auto rec_itr = proposal.begin();
        check( (rec_itr != proposal.end()),    "No proposal?!"); 
        check( (rec_itr -> expires_at > now() ), "proposal already expired.");  

        // whitelist_index white_list(get_self(), get_self().value);                    
        auto w_itr = white_list.find(votername.value); // Is the user on the list?        
        check((w_itr!=white_list.end()),     "votername not authorized by whitelist" );  
        check( (w_itr->vote==0),             "Second vote not allowed by the same voter!"); // diffrent than zero if already voted 
        check( (uservote==1)||(uservote==2), "Vote out of range. Should be 1 or 2!"); 

        //proposal and voter is valid at this place - update the vote
        white_list.modify(w_itr, get_self(),[&](auto& pp){
          pp.vote = uservote;
        });
        //-------------------------
        // Voting Results Analysis:

        /*      verify proposal approval or refusal   0-? 1-no 2-yes            Voting Table
                voter 1      voter 2                                  vote1 * vote2    meaning
                    0            0       - no one voted                     0          voting not finished
                    1            0       - only one voted                   0                ---
                    0            1       - only one voted                   0                ---
                    2            0       - only one voted                   0                ---
                    0            2       - only one voted                   0                --- 

                    1            1       - both refused                     1          finished and refused
                    2            1       - one refused                      2                ---
                    1            2       - one refused                      2                ---  

                    2            2       - both accepted                    4          finished and accepted */

        uint8_t vote1, vote2, voteresult;

        // Use secondary index to access all two voting results: 
        //  - if this is first voter voting now - the other result read will be 0.
        auto idx = white_list.get_index<"byidno"_n>();    
      
        auto iter = idx.lower_bound( VOTER1 ); // Table integrity is verified at the beginning of this action.
        vote1 = iter->vote; 
 
        iter = idx.find( VOTER2 );
        check( (iter != idx.end()), "VOTER2 not found!" ); // Seems to be not necessary as the table integrity is verified at the beginning
        vote2 = iter->vote;    

        voteresult = vote1 * vote2; //See the Voting Table above.
  
        if(voteresult!=0) // voting finished: accepted or refused 
        { 
          auto propr = proposal.begin();  
          if (voteresult==4)   // Mint NFT: 
            { // proposal accepted - finalize it :)

             if ( getclaimiteration() != 0 )  //If iteration=0 the replay() should be not called! (as it will not allow to mint new NFT). 
                replay(); // Enforce processing of outstanding (if any) iterations before minting new nft
                          //   to avoid 'extra past' profits for the new nft owner.
             else notify_front( ITERATION_ZERO ); // send warning on iteration zero to the frontend          
                                                                                                            
              // Copy proposal to the NFT register (this is newly minted NFT)
              nft_table nft_register( get_self(), get_self().value );
              auto it = nft_register.emplace(get_self(), [&]( auto& r ){
                r.nft_key                 = nft_register.available_primary_key(); 
                r.eosaccount              = propr->eosaccount; 
                r.roi_target_cap          = propr->roi_target_cap;
                r.nft_percentage          = propr->proposal_percentage;
                r.mint_date               = time_point_sec(current_time_point().sec_since_epoch());  
                r.locked                  = propr->locked;
                r.accrued                 = propr->accrued;  
                r.threshold               = propr->threshold;
                r.rates_left              = propr->rates_left;     
              });
              notify_front( NFT_MINTED );  // Info to frontend: 'proposal accepted and saved. NFT minted.'
            }
            else { 
              notify_front( NFT_REFUSED ); // proposal refused ( 0<voteresult<4 ) 
            }

            // Erase proposal in all cases when voting finished
            auto pro_itr = proposal.find(1);        
            proposal.modify(pro_itr, get_self(), [&](auto &p) 
            {
              p.eosaccount          = "empty"_n; 
              p.roi_target_cap      = 0;
              p.proposal_percentage = 0.0;
              p.threshold           = asset(0,symbol("POINT",4) ); 
              p.rates_left          = 0;
              p.locked              = true;
              p.accrued             = asset(0,symbol("POINT",4) ); 
              p.expires_at          = now();
            });

            // erase voting results in all cases when voting finished! 
            for( auto iter=white_list.begin(); iter!=white_list.end(); iter++ )
            {
              white_list.modify(iter, get_self(), [&]( auto& row )
              {
                row.vote = 0;
              }); 
            }
        } // end of voteresult!=0  
        // if voteresult==0 do nothing - voting not yet finished
} 
//
//---


  /*
   +-----------------------------------
   +  dryrun - dividend dry run 
   +-----------------------------------
             +
             +  Count proposal_percentage from all the avtive NFT's owned by a given person into list_by_user table.   
             */
  [[eosio::action]]
  void dividenda::dryrun( name proposername)   
  {
    // require_auth( proposername );
    // check( (auth_vip(proposername)==PROPOSER), "proposername not authorized by whitelist!" );
  
    // list_by_user - cumulative percentage paid for all active NFTs listed by user
    dryrun_data  list_by_user( get_self(), get_self().value );                                                                                         
    // erase 'list_by_user' before processing                   
    auto   rc_itr  =  list_by_user.begin();
    while (rc_itr !=  list_by_user.end()) 
    { 
       rc_itr =   list_by_user.erase(rc_itr);
    }

    // Create blank analytics table.      
    // The table keeps (for the last processed iteration)
    // cumulative nft_percentage for each of the categories: 
    // 1-iterative 2-horizontal 3-vertical for all active NFTs 

    uint8_t i;
    ewsanalytics analytics(get_self(), get_self().value ); 
    for( i=1; i<=3; i++ ){
      auto itr = analytics.find(i);
      if( itr == analytics.end() )
      {
        analytics.emplace(get_self(), [&](auto &p) { 
            p.category = i;
            p.bycategory    = 0.0; 
        });
      }
      else
      {
        analytics.modify(itr, get_self(), [&](auto &p) {
            p.category = i;
            p.bycategory    = 0.0; 
        });
      }     
    } //end of analytics table preparation

    // You are in DRY RUN //
 
    // Review the nft_register sequentially 
    nft_table nft_register( get_self(), get_self().value );

    for (auto iter = nft_register.begin(); iter != nft_register.end(); iter++)
    {       
      // extract working variables
            double   nft_percentage = iter->nft_percentage;
      const name     user           = iter->eosaccount;
      const bool     locked         = iter->locked;
            asset    accrued        = iter->accrued;
      const uint8_t  cap_type       = iter->roi_target_cap;
            asset    threshold      = iter->threshold;
            uint32_t ratesleft      = iter->rates_left; 

      // You are in DRY RUN 
      // Add current user proposal_percentage to correct analytics category
      // Note: analytics not count inactive nft_register - corrected instructions follows in appropriate cases.    

      switch( cap_type ) 
      {
        case ITERATION:   //WayFarer/investor - Iteration Cap
        {
          if (ratesleft>0)
            upsert_perc(ITERATION, user, nft_percentage);
          break;
        } 
          
        case HORIZONTAL:   //WayFarer/investor - Horizontal Cap
        {
          if( accrued.amount<=threshold.amount) 
            upsert_perc(HORIZONTAL, user, nft_percentage);
          break;
         }  

        case VERTICAL:   //WayFinder/Founder - Vertical Cap 
        {  
          if (!locked)
            upsert_perc(VERTICAL, user, nft_percentage); 
          break;
        } //end of case 3.
      } //end of switch.  

      }  //end of for (iter/nft_register)
  } // dryrun end
//
//---


  /*
   +-----------------------------------
   +  dividendhand - 
   +-----------------------------------
             +
             +  - serve one iteration dividend distribution  
             */

  void dividenda::dividendhand( uint64_t iter_point, asset profit )  // internal function 
  {
    //!< iter_point  - number currently processed (by me) iteration passed to the transfer memo
    //!< profit      - amount to share, taken from deposit table for current iteration - this part of optionsdiv account
    //!<               will be distributed in this iteration.
    // This should be called only as internal function, by other actions - it not verify any entrance conditions by itself.

    nft_table nft_register( get_self(), get_self().value );
    
    double all_spendings = 0.0; ///< Total percentage to be paid for all active NFTs on actually processed iteration.   
    double dao_dividend  = 0.0; ///< Remaining leftover percentage for DAO account ( dao_dividend = 100% - all_spendings ).
    // Read 'How much?' is to share on this round.
    asset pay_to_dao = profit; // from this will be subtracted NFT payments - the remaining will go to DAO.


    // receivers_list is collecting NFTs payments belonging to each user (user by user).
    recive_index receivers_list( get_self(), get_self().value ); ///< Rows summs payments to receive, by each user, from owned NFTs.
    // erase 'receivers_list' before processing!
    auto rc_itr    = receivers_list.begin();
    while (rc_itr != receivers_list.end()) 
    {  
       rc_itr =  receivers_list.erase(rc_itr);
    }

    // Review all the nft's from the register sequentially - count only payable (active). 
    //
    for (auto iter = nft_register.begin(); iter != nft_register.end(); iter++) // GO sequentially through all NFT's.
    {       
      // extract working variables from current NFT:
            double   nft_percentage = iter->nft_percentage;
      const name     user           = iter->eosaccount;
      const bool     locked         = iter->locked;
            asset    accrued        = iter->accrued;
      const uint8_t  cap_type       = iter->roi_target_cap;
            asset    threshold      = iter->threshold;
            uint32_t ratesleft      = iter->rates_left; 
            uint64_t nftkey         = iter->nft_key; //-- TEST --//

          // Count how much to pay for this NFT to the current user 
      // in POINT tokens:
      //old: // asset to_receive = asset(0,symbol("POINT",4) );          // v140
      //old: // to_receive = ( profit * (nft_percentage * 100)) / 10000; // v140
      asset to_receive = asset( (profit.amount * nft_percentage) / 100.0, profit.symbol); // v1.43

      switch( cap_type ) 
      {
   
        case ITERATION:   //WayFarer/investor - Iteration Cap
        {
            if (ratesleft>0) // NFT for the current user is active
            { // still paid 
              // Add percentage from this nft to the total percentage paid in this dividend round.
              // all_spendings += nft_percentage;
              pay_to_dao -= to_receive; // will be less for DAO :) 

              upsert_value( user, to_receive ); // to 'receivers_list'

              //update rates_left in nftx 
              nft_register.modify(iter,_self,[&](auto& p){ 
                p.rates_left = ratesleft - 1;  
              });
            }
            // else print("case 1 - no more rates left");
            break;
        } 
         
        case HORIZONTAL:   //WayFarer/investor - Horizontal Cap
        {     
              if( accrued.amount <= threshold.amount) // NFT for the current user is active
              { //stil paid
              // Add percentage from this nft to the total percentage paid in this dividend round.
              // Note! This is placed here as only active nftx should be counted in each switch case branch. 
              // all_spendings += nft_percentage;

                // correct payment if last (reduce to_receive)                                          
                asset overpayment = asset(0,symbol("POINT",4) );   
                asset simulated   = asset(0,symbol("POINT",4) );
                //simulated.amount  = (accrued.amount + to_receive.amount)/10000;
                simulated  = accrued + to_receive;

                // always last rate will have more or less overpayment over threshold - cut it here
                if ( simulated.amount > threshold.amount )
                { 
                  // this is the last rate to pay for this nftx.
                  // How much simulated is bigger than threshold?  
                  overpayment = simulated - threshold;    
                  to_receive = to_receive - overpayment;                    
                  //print("simulated: ", simulated, " overpayment= ", overpayment, "_to_receive=_", to_receive);
                }   //

                pay_to_dao -= to_receive; // will be less for DAO :) 
                
                upsert_value( user, to_receive ); // to 'receivers_list'

                nft_register.modify(iter,_self,[&](auto& p){
                    p.accrued = accrued + to_receive;
                });      
              } 
              break;
        }  

        case VERTICAL:   //WayFinder/Founder - Vertical Cap 
        {  
            if (!locked) // NFT for the current user is active
            {
              // Add percentage from this nft to the total percentage paid in this dividend round.
              // Note! This is placed here as only active nftx should be counted in each switch 
              // case branch. 
              all_spendings += nft_percentage;
              if(to_receive.amount>threshold.amount) 
              {
                to_receive = threshold;                   
              }
              pay_to_dao -= to_receive; // will be less for DAO :) 

              upsert_value( user, to_receive ); // to 'receivers_list'
            }  
            break;
        } //end of case VERTICAL.

      } //end of switch.     

    }    //end of for (iter/nft_register)
    //-------------------------------- 

    // Note: DAO account ('daoaccount') is added at the end of the receiver's list.   
    //--// asset to_receive = asset(0,symbol("POINT",4) ); 
    //--// to_receive = ( profit * (dao_dividend * 100)) / 10000; 
    auto idx = receivers_list.find(daoaccount.value); 
    if( idx == receivers_list.end() )
    {
      receivers_list.emplace( get_self(), [&]( auto& row ){
        row.user = daoaccount;
        row.to_receive = asset(0,symbol("POINT",4) ); 
        row.to_receive = pay_to_dao;
      });
    }
    else 
    { 
      receivers_list.modify(idx, get_self(), [&]( auto& row ) {  
           row.to_receive = pay_to_dao;         
      }); 
    } 
     

    //  The receivers_list table drives dividend transfers.
    //  Leftover is transferred to FreeDAO account.              
    //  The receivers_list is erased at the beginning of the action. 
    //
    for (auto idx = receivers_list.begin(); idx != receivers_list.end(); idx++)
    {
        name user = idx->user;
        asset quantity = asset(0,symbol("POINT",4) ); 
        quantity = idx->to_receive; 
        // std::string memo = std::string("period: ")+std::to_string( iter_point )+std::string(" dividend."); // v140
        std::string memo = std::string("Your week ")+std::to_string( iter_point )+std::string(" share of ")
        + profit.to_string(); // v1.43
        if(quantity.amount>0){
          action dtransfer = action(
            permission_level{get_self(),"active"_n},
            freeos_acct,   
            "allocate"_n,
            std::make_tuple(get_self(), user, quantity, memo )
          );
          dtransfer.send();
        }
    
    } //for  

  }  //end of action
//   
//---

  /*
   +-----------------------------------
   +  query -  
   +-----------------------------------
             +
             +  - Gives type of wallet active account for frontend.   
             */  

// Querying account type (0-3) for the frontend to automatically display correct web page.
// Answer is send through notify_front().   
[[eosio::action]]
void dividenda::query( name eosaccount )
{
  require_auth( eosaccount );
  // Clean up notify_front
  clearfront();                                                   
  // Identify account on whitelist (1,2, or 3) else (0)
  uint8_t answer;
  answer = auth_vip(eosaccount);
  notify_front(answer);
}  
//
//---

  /*
   +-----------------------------------
   +  dividcompute -  
   +-----------------------------------
             +
             +  Wrapper for replay() function to be called from outside.   
             */

[[eosio::action]]
void dividenda::dividcompute()
{
  replay(); 
}

  /*
   +-----------------------------------
   +  cron -  
   +-----------------------------------
             +
             +  Enables dividcompute to be called by Proton CRON service   
             */
[[eosio::action]]
void dividenda::cron()
{
  dividcompute(); 
}

//
//---

  /*
   +-----------------------------------
   +  replay -  
   +-----------------------------------
             +
             +  Process one by one the stack of not processed iterations.   
             */ 
// The current iteration is not processed. Iteration 0 (zero) is also not processed.  

void dividenda::replay()
{  
  deposit_index deposit_tbl(freeos_acct, freeos_acct.value);           //external deposit table 

  // read current iteration externally 
  uint64_t current_iter;                  // The row containing this iteration number cannot be processed! 
  current_iter = getclaimiteration();     // read externally  
  check( (current_iter!=0), "Current Iteration == 0 !!");
   
  // Process external deposit table from the beginning to the end sequentially. Exclude current iteration.  
    auto deposit_itr  = deposit_tbl.begin();
  while (deposit_itr != deposit_tbl.end()) {

    // Each item from the deposit table is examined and eventually processed (order has no meaning).
                                          // I need only to know the amount to divide taken from the row of the deposit table. 
    uint64_t iterpoint = deposit_itr->iteration;   // iteration number of the row under processing. 
    
    if( iterpoint!=current_iter ){  // Omit operation for this time only - row under processing is the same like current 
      asset itervalue = deposit_itr->accrued; // asset value of the row under processing. 

      dividendhand( iterpoint, itervalue );   // Process single iteration dividend. 
      // action to remove a deposit row from the deposit table
      action diverase = action(
        permission_level{ get_self(), "active"_n},
        freeos_acct,   
        "depositclear"_n,
         std::make_tuple( iterpoint )
      );
      diverase.send();
    }
    deposit_itr++;
  } //while 
}
//
//---

  /*
   +-----------------------------------
   +  notify_front -  
   +-----------------------------------
             +
             +  Build up a queue of warning messages for frontend.    
             */

void dividenda::notify_front( uint8_t number ) 
{
  messages_table errortable( get_self(), get_self().value );                                  
  auto ee = errortable.emplace( get_self(), [&](auto &e) {    
    e.key = errortable.available_primary_key(); 
    e.errorno = number;
  } );                                                                                      
} 
//
//---


// Clear frontend notification buffer created previously bu notify_front    
void dividenda::clearfront() {
    messages_table    errortable(get_self(), get_self().value);
    auto   rec_itr  = errortable.begin();
    while (rec_itr != errortable.end()) {
           rec_itr  = errortable.erase(rec_itr);
    }
} 
//
//---


  /*
   +-----------------------------------
   +  propreset -  
   +-----------------------------------
             +
             +  Withdraw active proposal.   
             */

[[eosio::action]] 
void dividenda::propreset( name proposername ) {   
  require_auth( proposername );
  check( (auth_vip(proposername)==PROPOSER), "proposername not authorized by whitelist!" );    

  // The proposer should be found and it should be first on the list. 
  // whitelist_index white_list(get_self(), get_self().value);
  // auto v = white_list.find(proposer.value);  // Is the proposer on the list?
  // check( (v!=white_list.end()), "No proposer on the list?!"); 
  // check( (v->idno==1), "On the list, but not the proposer!"); 
  
  //verify existence of the proposal record (if not exists create empty)
  proposal_table proposal(get_self(), get_self().value);
  auto pro_itr = proposal.begin();
  if( pro_itr == proposal.end() )
  {
     proposal.emplace(get_self(), [&](auto &p) { // This is only first time
        p.key                 = 1;
        p.eosaccount          = "empty"_n; 
        p.roi_target_cap      = 0;
        p.proposal_percentage = 0;
        p.threshold           = asset(0,symbol("POINT",4) ); 
        p.rates_left          = 0;
        p.locked              = false;
        p.accrued             = asset(0,symbol("POINT",4) ); 
        p.expires_at          = now();
     });
  }
  else {
    //auto pro_itr = proposal.begin();
    proposal.modify(pro_itr, get_self(), [&](auto &p) {
            p.eosaccount          = "erased"_n; 
            p.roi_target_cap      = 0;
            p.proposal_percentage = 0;
            p.threshold           = asset(0,symbol("POINT",4) ); 
            p.locked              = false;
            p.accrued             = asset(0,symbol("POINT",4) ); 
            p.expires_at          = now();
           });
  }   
  // erase remaining voting results!    v1.43
  whitelist_index white_list(get_self(), get_self().value);
  for( auto iter=white_list.begin(); iter!=white_list.end(); iter++ )
    {
      white_list.modify(iter, get_self(), [&]( auto& row )
      {
        row.vote = 0;
      }); 
    } // ... end of 1.43   
} 
//
//---

  /*
   +-----------------------------------
   +  regchown -  
   +-----------------------------------
             +
             +  Change NFT ownership.   
             */

[[eosio::action]]
void dividenda::regchown(name userfrom, name userto, uint64_t nft_key){
  require_auth(userfrom);

  check( userfrom != userto, "cannot transfer to self" );    
  check( is_account( userto ), "userto account does not exist");

  nft_table nft_register( get_self(), get_self().value );
  auto pro_itr = nft_register.find(nft_key);
  check(pro_itr != nft_register.end(), "NFT not found with the specified key"); // v2.2

  check( (pro_itr->eosaccount == userfrom ), "The NFT key does not agree with the owner account name!");

  if (pro_itr->roi_target_cap == ITERATION) {
     check(pro_itr->rates_left > 0, "You cannot change ownership of an expired NFT"); // v2.2
  }

  nft_register.modify(pro_itr, get_self(), [&](auto &p) {
            p.eosaccount = userto; 
  }); 
}
//
//---

  /*
   +-----------------------------------
   +  unlocknft - 
   +-----------------------------------
             +
             +  Unlock NFT lock for the founder (only for cap=3).   
             */

[[eosio::action]]
void dividenda::unlocknft( uint64_t nft_key ){
  check( nft_key > 0, "wrong or undefined NFT key ('nft_key')"); //nft_key is used because userfrom may have several nft_register
  // authorization is actually for proposer 
  whitelist_index white_list(get_self(), get_self().value);
     auto prx  = white_list.get_index<"byidno"_n>();
     auto itrx = prx.find( 1 );
  require_auth( itrx->user ); // The proposer is first on the whitelist. 

  nft_table nft_register( get_self(), get_self().value );
  auto pro_itr = nft_register.find(nft_key);
  if ( pro_itr->roi_target_cap == VERTICAL ){ // this should only work for cap=3  
  nft_register.modify(pro_itr, get_self(), [&](auto &p) {
            p.locked = false;  
  }); }
  else notify_front( NON_FOUNDER ); // notify frontend: 'action ignored: trying to unlock non-founder account'. 
}

// MAINTAIN
[[eosio::action]]
void dividenda::maintain( string action, name user ){

  require_auth(get_self());

  if (action == "modify nfts") {
    nft_table nft_register( get_self(), get_self().value );
    auto nft_iterator = nft_register.begin();

    while (nft_iterator != nft_register.end()) {
      nft_register.modify(nft_iterator, get_self(), [&](auto &p) {
            p.threshold = asset(1010000, symbol("POINT",4));
      });

      nft_iterator++;
    }
  }

  if (action == "remove nfts") {
    nft_table nft_register( get_self(), get_self().value );
    auto nft_iterator = nft_register.begin();

    while (nft_iterator != nft_register.end()) {
      nft_iterator = nft_register.erase(nft_iterator);
    }
  }

  if (action == "copy nfts") {
    // nfts table
    nft_table nft_register( get_self(), get_self().value );
    auto nft_iterator = nft_register.begin();

    // copynfts table
    copynft_table copynft_register( get_self(), get_self().value );

    while (nft_iterator != nft_register.end()) {
      copynft_register.emplace(get_self(), [&]( auto& r ){
                r.nft_key                 = nft_iterator->nft_key; 
                r.eosaccount              = nft_iterator->eosaccount; 
                r.roi_target_cap          = nft_iterator->roi_target_cap;
                r.nft_percentage          = nft_iterator->nft_percentage;
                r.mint_date               = nft_iterator->mint_date;  
                r.locked                  = nft_iterator->locked;
                r.accrued                 = nft_iterator->accrued;  
                r.threshold               = nft_iterator->threshold;
                r.rates_left              = nft_iterator->rates_left;     
              });

      nft_iterator++;
    }
  } // end of "copy nfts"


  if (action == "delete nfts") {
    // nfts table
    nft_table nft_register( get_self(), get_self().value );
    auto nft_iterator = nft_register.begin();

    while (nft_iterator != nft_register.end()) {
      nft_iterator = nft_register.erase(nft_iterator);
    }
  } // end of "delete nfts"


    if (action == "restore nfts") {
    // copynfts table
    copynft_table copynft_register( get_self(), get_self().value );
    auto copynft_iterator = copynft_register.begin();

    // nfts table
    nft_table nft_register( get_self(), get_self().value );

    while (copynft_iterator != copynft_register.end()) {
      nft_register.emplace(get_self(), [&]( auto& r ){
                r.nft_key                 = copynft_iterator->nft_key; 
                r.eosaccount              = copynft_iterator->eosaccount; 
                r.roi_target_cap          = copynft_iterator->roi_target_cap;
                r.nft_percentage          = copynft_iterator->nft_percentage;
                r.mint_date               = copynft_iterator->mint_date;  
                r.locked                  = copynft_iterator->locked;
                r.accrued                 = copynft_iterator->accrued;  
                r.threshold               = copynft_iterator->threshold;
                r.rates_left              = copynft_iterator->rates_left;     
              });

      copynft_iterator++;
    }
  } // end of "restore nfts"
  

  if (action == "delete copynfts") {
    // copynfts table
    copynft_table copynft_register( get_self(), get_self().value );
    auto copynft_iterator = copynft_register.begin();

    while (copynft_iterator != copynft_register.end()) {
      copynft_iterator = copynft_register.erase(copynft_iterator);
    }
  } // end of "delete copynfts"

}
//
//---

