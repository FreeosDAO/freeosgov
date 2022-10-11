#include "freebi.hpp"
#include "../tables.hpp"
#include "../constants.hpp"

namespace eosio {

   const std::string VERSION = "1.1.4";

// ACTION
void token::version() {

  std::string version_message = "version: " + VERSION + ", freeosgov account: " + freeosgov_acct;

  check(false, version_message);
}

/**
 * Function checks the `nft_table` in the `dividend` contract to see if the user has an active (unlocked) NFT
 * 
 * @param user the account name of the user
 * 
 * @return A boolean value indicating whether the user has an active NFT
 */
bool has_nft(name user) {
  bool nft_status = false;

  // check the dividend contract
  name dividend_contract = name(dividend_acct);

  freedao::nft_table nfts(dividend_contract, dividend_contract.value);
  auto account_index = nfts.get_index<"active"_n>();
  auto nft_iterator = account_index.find(user.value);

  if (nft_iterator != account_index.end()) {  // if NFT record found
    nft_status = true;
  }

  return nft_status;
}


void token::create( const name&   issuer,
                    const asset&  maximum_supply )
{
    require_auth( get_self() );

    auto sym = maximum_supply.symbol;
    check( sym.is_valid(), "invalid symbol name" );
    check( maximum_supply.is_valid(), "invalid supply");
    //check( maximum_supply.amount > 0, "max-supply must be positive"); // PROTON

    stats statstable( get_self(), sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    check( existing == statstable.end(), "token with symbol already exists" );

    statstable.emplace( get_self(), [&]( auto& s ) {
       s.supply.symbol = maximum_supply.symbol;
       // s.max_supply    = maximum_supply;    // PROTON
       s.max_supply    = asset{0, sym};        // PROTON
       s.issuer        = issuer;
    });
}


void token::issue( const name& to, const asset& quantity, const string& memo )
{
    auto sym = quantity.symbol;
    check( sym.is_valid(), "invalid symbol name" );
    check( memo.size() <= 256, "memo has more than 256 bytes" );

    stats statstable( get_self(), sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    check( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
    const auto& st = *existing;
    check( to == st.issuer, "tokens can only be issued to issuer account" );

    require_auth( st.issuer );
    check( quantity.is_valid(), "invalid quantity" );
    check( quantity.amount > 0, "must issue positive quantity" );

    check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    //check( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");  // PROTON
    check( quantity.amount + st.supply.amount <= MAX_AMOUNT, "quantity exceeds available supply");              // PROTON

    statstable.modify( st, same_payer, [&]( auto& s ) {
       s.max_supply = asset{0, st.supply.symbol};                    // PROTON
       s.supply += quantity;
    });

    add_balance( st.issuer, quantity, st.issuer );
}

void token::retire( const asset& quantity, const string& memo )
{
    auto sym = quantity.symbol;
    check( sym.is_valid(), "invalid symbol name" );
    check( memo.size() <= 256, "memo has more than 256 bytes" );

    stats statstable( get_self(), sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    check( existing != statstable.end(), "token with symbol does not exist" );
    const auto& st = *existing;

    require_auth( st.issuer );
    check( quantity.is_valid(), "invalid quantity" );
    check( quantity.amount > 0, "must retire positive quantity" );

    check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

    statstable.modify( st, same_payer, [&]( auto& s ) {
       s.supply -= quantity;
    });

    sub_balance( st.issuer, quantity );
}

// burn is a copy of retire without requiring the authority of token issuer
// The function is not an action. It is called internally as part of the (modified) transfer action. 
void token::burn( const asset& quantity, const string& memo )
{
    auto sym = quantity.symbol;
    check( sym.is_valid(), "invalid symbol name" );
    check( memo.size() <= 256, "memo has more than 256 bytes" );

    stats statstable( get_self(), sym.code().raw() );
    auto existing = statstable.find( sym.code().raw() );
    check( existing != statstable.end(), "token with symbol does not exist" );
    const auto& st = *existing;

    check( quantity.is_valid(), "invalid quantity" );
    check( quantity.amount > 0, "must retire positive quantity" );

    check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

    statstable.modify( st, same_payer, [&]( auto& s ) {
       s.supply -= quantity;
    });

    sub_balance( st.issuer, quantity );
}

asset token::calculate_fee(const name &from, const asset& transfer_quantity)
{
   asset fee = asset(0, FREEBI_CURRENCY_SYMBOL);   // default value
   
   name freeoscontract = name(freeosgov_acct);

   // the transfer fee is only charged for user-to-user transfers - do not charge if the transfer is from a contract
   if (from != freeoscontract && from != get_self()) {
      name fee_parameter = name("freebixfee");
   
      freedao::dparameters_index dparameters_table(freeoscontract, freeoscontract.value);
      auto dparameter_iterator = dparameters_table.find(fee_parameter.value);
      check(dparameter_iterator != dparameters_table.end(), fee_parameter.to_string() + " is not defined");
      double fee_percent = dparameter_iterator->value;

      int64_t fee_units = transfer_quantity.amount * (fee_percent / 100.0);
      fee = asset(fee_units, FREEBI_CURRENCY_SYMBOL);
   }   
   
   return fee;
}

void token::transfer( const name&    from,
                      const name&    to,
                      const asset&   quantity,
                      const string&  memo )
{
    check( from != to, "cannot transfer to self" );
    require_auth( from );
    check( is_account( to ), "to account does not exist");

    // check if 'to' account is regsistered in freeosgov with a verified account OR is the freeosgov account
    if (to != name(freeosgov_acct)) {
      freedao::participants_index users_table(name(freeosgov_acct), to.value);
      auto user_iterator = users_table.begin();
      check(user_iterator != users_table.end(), "the recipient must be a registered Freeos user");
      check(user_iterator->account_type == "v" || user_iterator->account_type == "b" || user_iterator->account_type == "c" || has_nft(to), "the recipient must be a verified Freeos user");
    }

    auto sym = quantity.symbol.code();
    stats statstable( get_self(), sym.raw() );
    const auto& st = statstable.get( sym.raw() );

    require_recipient( from );
    require_recipient( to );

    check( quantity.is_valid(), "invalid quantity" );
    check( quantity.amount > 0, "must transfer positive quantity" );
    check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    check( memo.size() <= 256, "memo has more than 256 bytes" );

    // calculate the transfer fee amount to burn
    asset fee_quantity = calculate_fee(from, quantity);

    asset transfer_quantity = quantity - fee_quantity;
    
    auto payer = has_auth( to ) ? to : from;

    sub_balance( from, transfer_quantity );
    add_balance( to, transfer_quantity, payer );

    if (fee_quantity.amount > 0) {
      // transfer the fee to the issuer account and then burn it
      sub_balance( from, fee_quantity );
      add_balance( name(freeosgov_acct), fee_quantity, payer );

      burn(fee_quantity, "FREEBI transfer fee");
    }
    
}

void token::sub_balance( const name& owner, const asset& value ) {
   accounts from_acnts( get_self(), owner.value );

   const auto& from = from_acnts.get( value.symbol.code().raw(), "no balance object found" );
   check( from.balance.amount >= value.amount, "overdrawn balance" );

   from_acnts.modify( from, owner, [&]( auto& a ) {
         a.balance -= value;
      });
}

void token::add_balance( const name& owner, const asset& value, const name& ram_payer )
{
   accounts to_acnts( get_self(), owner.value );
   auto to = to_acnts.find( value.symbol.code().raw() );
   if( to == to_acnts.end() ) {
      to_acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = value;
      });
   } else {
      to_acnts.modify( to, same_payer, [&]( auto& a ) {
        a.balance += value;
      });
   }
}

void token::open( const name& owner, const symbol& symbol, const name& ram_payer )
{
   require_auth( ram_payer );

   check( is_account( owner ), "owner account does not exist" );

   auto sym_code_raw = symbol.code().raw();
   stats statstable( get_self(), sym_code_raw );
   const auto& st = statstable.get( sym_code_raw, "symbol does not exist" );
   check( st.supply.symbol == symbol, "symbol precision mismatch" );

   accounts acnts( get_self(), owner.value );
   auto it = acnts.find( sym_code_raw );
   if( it == acnts.end() ) {
      acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = asset{0, symbol};
      });
   }
}

void token::close( const name& owner, const symbol& symbol )
{
   require_auth( owner );
   accounts acnts( get_self(), owner.value );
   auto it = acnts.find( symbol.code().raw() );
   check( it != acnts.end(), "Balance row already deleted or never existed. Action won't have any effect." );
   check( it->balance.amount == 0, "Cannot close because the balance is not zero." );
   acnts.erase( it );
}

} /// namespace eosio
