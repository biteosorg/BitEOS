/**
 *  @file
 *  @copyright defined in bes/LICENSE.txt
 */
#include "besio.platform.hpp"

#include <besiolib/besio.hpp>
#include <besiolib/print.hpp>
#include <besiolib/datastream.hpp>
#include <besiolib/serialize.hpp>
#include <besiolib/multi_index.hpp>
#include <besiolib/privileged.h>
#include <besiolib/transaction.hpp>

#include <besio.token/besio.token.hpp>


#include <cmath>
#include <map>

namespace besioplatform {
   using besio::asset;
   using besio::indexed_by;
   using besio::const_mem_fun;
   using besio::bytes;
   using besio::print;
   using besio::permission_level;
   using std::map;
   using std::pair;

   static constexpr time refund_delay = 3*24*3600;
   static constexpr time refund_expiration_time = 3600;

   /**
    *  Every user 'from' has a scope/table that uses every receipient 'to' as the primary key.
    */
   struct delegated_bandwidth {
      account_name  from;
      account_name  to;
      asset         net_weight;
      asset         cpu_weight;

      uint64_t  primary_key()const { return to; }

      // explicit serialization macro is not necessary, used here only to improve compilation time
      BESLIB_SERIALIZE( delegated_bandwidth, (from)(to)(net_weight)(cpu_weight) )

   };

   struct refund_request {
      account_name  owner;
      time          request_time;
      besio::asset  net_amount;
      besio::asset  cpu_amount;

      uint64_t  primary_key()const { return owner; }

      // explicit serialization macro is not necessary, used here only to improve compilation time
      BESLIB_SERIALIZE( refund_request, (owner)(request_time)(net_amount)(cpu_amount) )
   };

   /**
    *  These tables are designed to be constructed in the scope of the relevant user, this
    *  facilitates simpler API for per-user queries
    */
   typedef besio::multi_index< N(userres), user_resources>      user_resources_table;
   typedef besio::multi_index< N(delband), delegated_bandwidth> del_bandwidth_table;
   typedef besio::multi_index< N(refunds), refund_request>      refunds_table;



   /**
    *  This action will buy an exact amount of ram and bill the payer the current market price.
    */
   void platform_contract::buyrambytes( account_name payer, account_name receiver, uint32_t bytes ) {
      auto itr = _rammarket.find(S(4,RAMCORE));
      auto tmp = *itr;
      auto besout = tmp.convert( asset(bytes,S(0,RAM)), CORE_SYMBOL );

      buyram( payer, receiver, besout );
   }


   /**
    *  When buying ram the payer irreversiblly transfers quant to platform contract and only
    *  the receiver may reclaim the tokens via the sellram action. The receiver pays for the
    *  storage of all database records associated with this action.
    *
    *  RAM is a scarce resource whose supply is defined by global properties max_ram_size. RAM is
    *  priced using the bancor algorithm such that price-per-byte with a constant reserve ratio of 100:1.
    */
   void platform_contract::buyram( account_name payer, account_name receiver, asset quant )
   {
      print("------------------------------buyram-----------------------\n");

      require_auth( payer );
      besio_assert( quant.amount > 0, "must purchase a positive amount" );
      
      double fee_rate = get_ram_trad_fee_rate( receiver );
      print("fee_rate = ", fee_rate, "\n");
      int64_t fee = static_cast<int64_t>(quant.amount * fee_rate);

      // fee.amount cannot be 0 since that is only possible if quant.amount is 0 which is not allowed by the assert above.
      // If quant.amount == 1, then fee.amount == 1,
      // otherwise if quant.amount > 1, then 0 < fee.amount < quant.amount.
      auto quant_after_fee = quant;
      quant_after_fee.amount -= fee;
      // quant_after_fee.amount should be > 0 if quant.amount > 1.
      // If quant.amount == 1, then quant_after_fee.amount == 0 and the next inline transfer will fail causing the buyram action to fail.

      INLINE_ACTION_SENDER(besio::token, transfer)( N(besio.token), {payer,N(active)},
         { payer, N(besio.ram), quant_after_fee, std::string("buy ram") } );

      if( fee > 0 ) {
         INLINE_ACTION_SENDER(besio::token, transfer)( N(besio.token), {payer,N(active)},
                                                       { payer, N(besio.ramfee), asset(fee), std::string("ram fee") } );
      }

      int64_t bytes_out;

      const auto& market = _rammarket.get(S(4,RAMCORE), "ram market does not exist");
      _rammarket.modify( market, 0, [&]( auto& es ) {
          bytes_out = es.convert( quant_after_fee,  S(0,RAM) ).amount;
      });

      besio_assert( bytes_out > 0, "must reserve a positive amount" );

      _gstate.total_ram_bytes_reserved += uint64_t(bytes_out);
      _gstate.total_ram_stake          += quant_after_fee.amount;

      user_resources_table  userres( _self, receiver );
      auto res_itr = userres.find( receiver );
      if( res_itr ==  userres.end() ) {
         res_itr = userres.emplace( receiver, [&]( auto& res ) {
               res.owner = receiver;
               res.ram_bytes = bytes_out;
            });
      } else {
         userres.modify( res_itr, receiver, [&]( auto& res ) {
               res.ram_bytes += bytes_out;
            });
      }
      set_resource_limits( res_itr->owner, res_itr->ram_bytes, res_itr->net_weight.amount, res_itr->cpu_weight.amount );

      upramuseage(receiver);
   }


   /**
    *  The platform contract now buys and sells RAM allocations at prevailing market prices.
    *  This may result in traders buying RAM today in anticipation of potential shortages
    *  tomorrow. Overall this will result in the market balancing the supply and demand
    *  for RAM over time.
    */
   void platform_contract::sellram( account_name account, int64_t bytes ) {
      require_auth( account );

      print("------------------------------sellram-----------------------\n");
      double fee_rate = get_ram_trad_fee_rate( account );
      print("fee_rate = ", fee_rate, "\n");

      besio_assert( bytes > 0, "cannot sell negative byte" );

      user_resources_table  userres( _self, account );
      auto res_itr = userres.find( account );
      besio_assert( res_itr != userres.end(), "no resource row" );
      besio_assert( res_itr->ram_bytes >= bytes, "insufficient quota" );

      asset tokens_out;
      auto itr = _rammarket.find(S(4,RAMCORE));
      _rammarket.modify( itr, 0, [&]( auto& es ) {
          /// the cast to int64_t of bytes is safe because we certify bytes is <= quota which is limited by prior purchases
          tokens_out = es.convert( asset(bytes,S(0,RAM)), CORE_SYMBOL);
      });

      besio_assert( tokens_out.amount > 1, "token amount received from selling ram is too low" );

      _gstate.total_ram_bytes_reserved -= static_cast<decltype(_gstate.total_ram_bytes_reserved)>(bytes); // bytes > 0 is asserted above
      _gstate.total_ram_stake          -= tokens_out.amount;

      //// this shouldn't happen, but just in case it does we should prevent it
      besio_assert( _gstate.total_ram_stake >= 0, "error, attempt to unstake more tokens than previously staked" );

      userres.modify( res_itr, account, [&]( auto& res ) {
          res.ram_bytes -= bytes;
      });
      set_resource_limits( res_itr->owner, res_itr->ram_bytes, res_itr->net_weight.amount, res_itr->cpu_weight.amount );

      int64_t fee = static_cast<int64_t>( tokens_out.amount * fee_rate ); // dynamic fee (round up) 
      // since tokens_out.amount was asserted to be at least 2 earlier, fee.amount < tokens_out.amount
      
      set_resource_limits( res_itr->owner, res_itr->ram_bytes, res_itr->net_weight.amount, res_itr->cpu_weight.amount );

      INLINE_ACTION_SENDER(besio::token, transfer)( N(besio.token), {N(besio.ram),N(active)},
                                                       { N(besio.ram), account, asset(tokens_out), std::string("sell ram") } );

      // since tokens_out.amount was asserted to be at least 2 earlier, fee.amount < tokens_out.amount
      
      if( fee > 0 ) {
         INLINE_ACTION_SENDER(besio::token, transfer)( N(besio.token), {account,N(active)},
            { account, N(besio.ramfee), asset(fee), std::string("sell ram fee") } );
      }
      
      upramuseage(account);
   }

   void platform_contract::changebw( account_name from, account_name receiver,
                                   const asset stake_net_delta, const asset stake_cpu_delta, bool transfer )
   {
      require_auth( from );
      besio_assert( stake_net_delta != asset(0) || stake_cpu_delta != asset(0), "should stake non-zero amount" );
      besio_assert( std::abs( (stake_net_delta + stake_cpu_delta).amount )
                     >= std::max( std::abs( stake_net_delta.amount ), std::abs( stake_cpu_delta.amount ) ),
                    "net and cpu deltas cannot be opposite signs" );

      account_name source_stake_from = from;
      if ( transfer ) {
         from = receiver;
      }

      // update stake delegated from "from" to "receiver"
      {
         del_bandwidth_table     del_tbl( _self, from);
         auto itr = del_tbl.find( receiver );
         if( itr == del_tbl.end() ) {
            itr = del_tbl.emplace( from, [&]( auto& dbo ){
                  dbo.from          = from;
                  dbo.to            = receiver;
                  dbo.net_weight    = stake_net_delta;
                  dbo.cpu_weight    = stake_cpu_delta;
               });
         }
         else {
            del_tbl.modify( itr, 0, [&]( auto& dbo ){
                  dbo.net_weight    += stake_net_delta;
                  dbo.cpu_weight    += stake_cpu_delta;
               });
         }
         besio_assert( asset(0) <= itr->net_weight, "insufficient staked net bandwidth" );
         besio_assert( asset(0) <= itr->cpu_weight, "insufficient staked cpu bandwidth" );
         if ( itr->net_weight == asset(0) && itr->cpu_weight == asset(0) ) {
            del_tbl.erase( itr );
         }
      } // itr can be invalid, should go out of scope

      // update totals of "receiver"
      {
         user_resources_table   totals_tbl( _self, receiver );
         auto tot_itr = totals_tbl.find( receiver );
         if( tot_itr ==  totals_tbl.end() ) {
            tot_itr = totals_tbl.emplace( from, [&]( auto& tot ) {
                  tot.owner = receiver;
                  tot.net_weight    = stake_net_delta;
                  tot.cpu_weight    = stake_cpu_delta;
               });
         } else {
            totals_tbl.modify( tot_itr, from == receiver ? from : 0, [&]( auto& tot ) {
                  tot.net_weight    += stake_net_delta;
                  tot.cpu_weight    += stake_cpu_delta;
               });
         }
         besio_assert( asset(0) <= tot_itr->net_weight, "insufficient staked total net bandwidth" );
         besio_assert( asset(0) <= tot_itr->cpu_weight, "insufficient staked total cpu bandwidth" );

         set_resource_limits( receiver, tot_itr->ram_bytes, tot_itr->net_weight.amount, tot_itr->cpu_weight.amount );

         if ( tot_itr->net_weight == asset(0) && tot_itr->cpu_weight == asset(0)  && tot_itr->ram_bytes == 0 ) {
            totals_tbl.erase( tot_itr );
         }
      } // tot_itr can be invalid, should go out of scope

      // create refund or update from existing refund
      if ( N(besio.stake) != source_stake_from ) { //for besio both transfer and refund make no sense
         refunds_table refunds_tbl( _self, from );
         auto req = refunds_tbl.find( from );

         //create/update/delete refund
         auto net_balance = stake_net_delta;
         auto cpu_balance = stake_cpu_delta;
         bool need_deferred_trx = false;


         // net and cpu are same sign by assertions in delegatebw and undelegatebw
         // redundant assertion also at start of changebw to protect against misuse of changebw
         bool is_undelegating = (net_balance.amount + cpu_balance.amount ) < 0;
         bool is_delegating_to_self = (!transfer && from == receiver);

         if( is_delegating_to_self || is_undelegating ) {
            if ( req != refunds_tbl.end() ) { //need to update refund
               refunds_tbl.modify( req, 0, [&]( refund_request& r ) {
                  if ( net_balance < asset(0) || cpu_balance < asset(0) ) {
                     r.request_time = now();
                  }
                  r.net_amount -= net_balance;
                  if ( r.net_amount < asset(0) ) {
                     net_balance = -r.net_amount;
                     r.net_amount = asset(0);
                  } else {
                     net_balance = asset(0);
                  }
                  r.cpu_amount -= cpu_balance;
                  if ( r.cpu_amount < asset(0) ){
                     cpu_balance = -r.cpu_amount;
                     r.cpu_amount = asset(0);
                  } else {
                     cpu_balance = asset(0);
                  }
               });

               besio_assert( asset(0) <= req->net_amount, "negative net refund amount" ); //should never happen
               besio_assert( asset(0) <= req->cpu_amount, "negative cpu refund amount" ); //should never happen

               if ( req->net_amount == asset(0) && req->cpu_amount == asset(0) ) {
                  refunds_tbl.erase( req );
                  need_deferred_trx = false;
               } else {
                  need_deferred_trx = true;
               }

            } else if ( net_balance < asset(0) || cpu_balance < asset(0) ) { //need to create refund
               refunds_tbl.emplace( from, [&]( refund_request& r ) {
                  r.owner = from;
                  if ( net_balance < asset(0) ) {
                     r.net_amount = -net_balance;
                     net_balance = asset(0);
                  } // else r.net_amount = 0 by default constructor
                  if ( cpu_balance < asset(0) ) {
                     r.cpu_amount = -cpu_balance;
                     cpu_balance = asset(0);
                  } // else r.cpu_amount = 0 by default constructor
                  r.request_time = now();
               });
               need_deferred_trx = true;
            } // else stake increase requested with no existing row in refunds_tbl -> nothing to do with refunds_tbl
         } /// end if is_delegating_to_self || is_undelegating

         if ( need_deferred_trx ) {
            besio::transaction out;
            out.actions.emplace_back( permission_level{ from, N(active) }, _self, N(refund), from );
            out.delay_sec = refund_delay;
            cancel_deferred( from ); // TODO: Remove this line when replacing deferred trxs is fixed
            out.send( from, from, true );
         } else {
            cancel_deferred( from );
         }

         auto transfer_amount = net_balance + cpu_balance;
         if ( asset(0) < transfer_amount ) {
            INLINE_ACTION_SENDER(besio::token, transfer)( N(besio.token), {source_stake_from, N(active)},
               { source_stake_from, N(besio.stake), asset(transfer_amount), std::string("stake bandwidth") } );
         }
      }

      /**
       * Update the number of mortgages available for voting
       * The user can choose to mortgage the voter after mortgage
       * If the user is first mortgaged, a voting account is created
       * If voters reduce their mortgages, they can only reduce the remainder of their current mortgages
       * If voters increase their mortgages, they only increase the remainder
      */
      {
         asset total_update = stake_net_delta + stake_cpu_delta;
         auto from_voter = _voters.find(from);
         if( from_voter == _voters.end() ) {
            from_voter = _voters.emplace( from, [&]( auto& v ) {
                  v.votername  = from;
                  v.staked_balance = total_update;
               });
         } else {
            _voters.modify( from_voter, 0, [&]( auto& v ) {
                  v.staked_balance += total_update;
               });
         }
         besio_assert( from_voter->staked_balance.amount >= 0 , "stake for voting cannot be negative");
         print("vetor:", name{from_voter->votername}, " staked balance is ", from_voter->staked_balance.amount, "\n");
      }
      
   }

   void platform_contract::delegatebw( account_name from, account_name receiver,
                                     asset stake_net_quantity,
                                     asset stake_cpu_quantity, bool transfer )
   {
      besio_assert( stake_cpu_quantity >= asset(0), "must stake a positive amount" );
      besio_assert( stake_net_quantity >= asset(0), "must stake a positive amount" );
      besio_assert( stake_net_quantity + stake_cpu_quantity > asset(0), "must stake a positive amount" );
      besio_assert( !transfer || from != receiver, "cannot use transfer flag if delegating to self" );

      changebw( from, receiver, stake_net_quantity, stake_cpu_quantity, transfer);
   } // delegatebw

   void platform_contract::undelegatebw( account_name from, account_name receiver,
                                       asset unstake_net_quantity, asset unstake_cpu_quantity )
   {
      besio_assert( asset() <= unstake_cpu_quantity, "must unstake a positive amount" );
      besio_assert( asset() <= unstake_net_quantity, "must unstake a positive amount" );
      besio_assert( asset() < unstake_cpu_quantity + unstake_net_quantity, "must unstake a positive amount" );
      besio_assert( _gstate.total_activated_votes >= min_activated_votes,
                    "cannot undelegate bandwidth until the chain is activated (at least 15% of all tokens participate in voting)" );

      changebw( from, receiver, -unstake_net_quantity, -unstake_cpu_quantity, false);
   } // undelegatebw


   void platform_contract::refund( const account_name owner ) {
      require_auth( owner );

      refunds_table refunds_tbl( _self, owner );
      auto req = refunds_tbl.find( owner );
      besio_assert( req != refunds_tbl.end(), "refund request not found" );
      besio_assert( req->request_time + refund_delay <= now(), "refund is not available yet" );
      // Until now() becomes NOW, the fact that now() is the timestamp of the previous block could in theory
      // allow people to get their tokens earlier than the 3 day delay if the unstake happened immediately after many
      // consecutive missed blocks.

      INLINE_ACTION_SENDER(besio::token, transfer)( N(besio.token), {N(besio.stake),N(active)},
                                                    { N(besio.stake), req->owner, req->net_amount + req->cpu_amount, std::string("unstake") } );

      refunds_tbl.erase( req );
   }

   void platform_contract::upramuseage(account_name owner) {

      user_resources_table  userres( _self, owner );
      auto res_itr = userres.find( owner );
      besio_assert( res_itr != userres.end(), "user must first register" );

      user_resources res_obj;
      res_obj.owner = res_itr->owner;
      res_obj.ram_trading_fee_rate = res_itr->ram_trading_fee_rate;
      res_obj.ram_last_warning_time = res_itr->ram_last_warning_time;
      res_obj.ram_info_last_update_time = res_itr->ram_info_last_update_time;
      
      cal_ram_real_time_trad_fee_rate(res_obj);

      userres.modify( res_itr, owner, [&]( auto& res ) {
          res.ram_trading_fee_rate = res_obj.ram_trading_fee_rate;
          print("upramuseage---- res.ram_trading_fee_rate =", res.ram_trading_fee_rate,  "\n");
          res.ram_last_warning_time = res_obj.ram_last_warning_time;
          print("upramuseage---- res.ram_last_warning_time =", res.ram_last_warning_time,  "\n");
          res.ram_info_last_update_time = res_obj.ram_info_last_update_time;
          print("upramuseage---- res.ram_info_last_update_time =", res.ram_info_last_update_time,  "\n");
      });
   }

   double platform_contract::cal_ram_real_time_trad_fee_rate(user_resources & resobj) {

      int64_t ram_own_bytes = 0;
      int64_t ram_usage_bytes = 0;
      int64_t ram_unuse_bytes = 0;
      print("cal_ram_trad_fee_rate---- account:", name{ resobj.owner }, "\n");
      get_ram_usage(resobj.owner, ram_own_bytes, ram_usage_bytes);
      print("cal_ram_trad_fee_rate---- get_ram_usage fun load,ram_own_bytes=", ram_own_bytes, " ram_usage_bytes=", ram_usage_bytes,  "\n");
      
      double ram_unuse_rate = 0;
      /** first by ram with inline action */
      if ( 0 != ram_own_bytes ) {
         ram_unuse_bytes = ram_own_bytes - ram_usage_bytes;
         besio_assert( ram_unuse_bytes > 0, "account has insufficient ram" );
         
         ram_unuse_rate = double(ram_unuse_bytes) / double(ram_own_bytes );
      }

      double ram_trading_fee_rate = 0;
      uint64_t ram_last_warning_time = 0;
      uint64_t ct = current_time();

      print("cal_ram_trad_fee_rate---- ram_unuse_rate=", ram_unuse_rate,  "\n");

      if(ram_unuse_rate > ram_punitive_unuse_rate) {
         if( 0 == resobj.ram_last_warning_time ){
            ram_trading_fee_rate = ram_trade_basis_fee_rate;
            ram_last_warning_time = ct;
            print("cal_ram_trad_fee_rate---- ram_trading_fee_rate=", ram_trading_fee_rate,  "\n");
            print("cal_ram_trad_fee_rate---- ram_last_warning_time=", ram_last_warning_time,  "\n");
         }else{
            if ( ((ct - resobj.ram_last_warning_time) / ram_trade_punitive_fee_preiod) >= ram_trade_punitive_fee_warning_period ) {
		       ram_trading_fee_rate = ram_trade_max_fee_rate* exp(-10 / ( ram_unuse_rate*((ct - resobj.ram_last_warning_time) / ram_trade_punitive_fee_preiod) ) );
               if ( ram_trading_fee_rate < ram_trade_basis_fee_rate ) {
                  ram_trading_fee_rate = ram_trade_basis_fee_rate;
               }
            }else{
               ram_trading_fee_rate = ram_trade_basis_fee_rate;
            }
            print("cal_ram_trad_fee_rate---- warning ram_trading_fee_rate=", ram_trading_fee_rate,  "\n");
            ram_last_warning_time = resobj.ram_last_warning_time;
         }
      }
      else {
         ram_trading_fee_rate = ram_trade_basis_fee_rate;
      }
      resobj.ram_trading_fee_rate = static_cast<uint32_t>( ram_trading_fee_rate * 10000 );
      print("cal_ram_trad_fee_rate---- res.ram_trading_fee_rate =", resobj.ram_trading_fee_rate,  "\n");
      resobj.ram_last_warning_time = ram_last_warning_time;
      print("cal_ram_trad_fee_rate---- res.ram_last_warning_time =", resobj.ram_last_warning_time,  "\n");
      resobj.ram_info_last_update_time = ct;
      print("cal_ram_trad_fee_rate---- res.ram_info_last_update_time =", resobj.ram_info_last_update_time,  "\n");

      return ram_trading_fee_rate;
   }

   double platform_contract::get_ram_trad_fee_rate(account_name account) {
      user_resources_table  userres( _self, account );
      print("get_ram_trad_fee_rate---- account:", name{ account }, "\n");
      auto res_itr = userres.find( account );
      if ( res_itr == userres.end() ) {
         return ram_trade_basis_fee_rate;
      }
      else {
         user_resources res_obj;
         res_obj.owner = res_itr->owner;
         res_obj.ram_trading_fee_rate = res_itr->ram_trading_fee_rate;
         res_obj.ram_last_warning_time = res_itr->ram_last_warning_time;
         res_obj.ram_info_last_update_time = res_itr->ram_info_last_update_time;

         print("get_ram_trad_fee_rate---- upramuseage fun load\n");
         double ram_trade_fee_rate = cal_ram_real_time_trad_fee_rate(res_obj);
         print("get_ram_trad_fee_rate---- ram_trade_fee_rate", ram_trade_fee_rate, "\n");
         return ram_trade_fee_rate;
      }
   }

} //namespace besioplatform
