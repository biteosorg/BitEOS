/**
 *  @file
 *  @copyright defined in bes/LICENSE.txt
 */
#include "besio.platform.hpp"

#include <besiolib/besio.hpp>
#include <besiolib/crypto.h>
#include <besiolib/print.hpp>
#include <besiolib/datastream.hpp>
#include <besiolib/serialize.hpp>
#include <besiolib/multi_index.hpp>
#include <besiolib/privileged.hpp>
#include <besiolib/singleton.hpp>
#include <besiolib/transaction.hpp>
#include <besio.token/besio.token.hpp>

#include <algorithm>
#include <cmath>

namespace besioplatform {
   using besio::indexed_by;
   using besio::const_mem_fun;
   using besio::bytes;
   using besio::print;
   using besio::singleton;
   using besio::transaction;

   /**
    *  This method will create a producer_config and producer_info object for 'producer'
    *
    *  @pre producer is not already registered
    *  @pre producer to register is an account
    *  @pre authority of producer to register
    *
    */
   void platform_contract::regproducer( const account_name producer, const besio::public_key& producer_key, const std::string& url, uint16_t location, uint32_t commission_rate ) {
      besio_assert( url.size() < 512, "url too long" );
      besio_assert( producer_key != besio::public_key(), "public key should not be the default value" );
      besio_assert( 1 <= commission_rate && commission_rate <= 10000, "commission rate should >=1 and <= 10000" );
      require_auth( producer );

      auto distribut_time = current_time();

      auto prod = _producers.find( producer );
            
      if ( prod != _producers.end() ) {
         /** check the producer's commission_rate adjustment */
         if( prod->commission_rate != commission_rate ){

            besio_assert( distribut_time - prod->last_commission_rate_adjustment_time > min_commission_adjustment_period, "The commission ratio has been adjusted, please try again later" );
            
            /** If the commission ratio is reduced, the ratio needs to be met */
            if( prod->commission_rate > commission_rate ){
               auto adjustment_rate = static_cast<double>((commission_rate - prod->commission_rate) / double(prod->commission_rate));
               besio_assert( adjustment_rate <= max_commission_adjustment_rate,
               "The commission ratio does not meet the adjustment requirements. Please try again after adjustment"); 
            }

            /** Give rewards to voters, but only modify the value of the rewards */
            distribute_rewards(distribut_time);

            _producers.modify( prod, producer, [&]( producer_info& info ){
               info.producer_key = producer_key;
               info.is_active    = true;
               info.url          = url;
               info.location     = location;
               info.commission_rate = commission_rate;
               info.last_commission_rate_adjustment_time = distribut_time;
            });
         }else{
            _producers.modify( prod, producer, [&]( producer_info& info ){
               info.producer_key = producer_key;
               info.is_active    = true;
               info.url          = url;
               info.location     = location;
            }); 
         }

      } else {
         _producers.emplace( producer, [&]( producer_info& info ){
               info.owner         = producer;
               info.total_votes   = 0;
               info.producer_key  = producer_key;
               info.is_active     = true;
               info.url           = url;
               info.location      = location;
               info.commission_rate = commission_rate;
               info.last_commission_rate_adjustment_time = distribut_time;
         });
      }
   }

   void platform_contract::unregprod( const account_name producer ) {
      require_auth( producer );

      const auto& prod = _producers.get( producer, "producer not found" );
      
      /** Give rewards to voters, but only modify the value of the rewards */
      auto distribut_time = current_time();
      distribute_rewards(distribut_time);

      _producers.modify( prod, 0, [&]( producer_info& info ){
            info.deactivate();
      });
   }

   void platform_contract::update_elected_producers( block_timestamp block_time ) {
      _gstate.last_producer_schedule_update = block_time;

      auto idx = _producers.get_index<N(prototalvote)>();

      std::vector< std::pair<besio::producer_key,uint16_t> > top_producers;
      top_producers.reserve(SUPER_NODES_NUM);

      for ( auto it = idx.cbegin(); it != idx.cend() && top_producers.size() < SUPER_NODES_NUM && 0 < it->total_votes && it->active(); ++it ) {
         top_producers.emplace_back( std::pair<besio::producer_key,uint16_t>({{it->owner, it->producer_key}, it->location}) );
      }

      if ( top_producers.size() < _gstate.last_producer_schedule_size ) {
         return;
      }

      /// sort by producer name
      std::sort( top_producers.begin(), top_producers.end() );

      std::vector<besio::producer_key> producers;

      producers.reserve(top_producers.size());
      for( const auto& item : top_producers )
         producers.push_back(item.first);

      bytes packed_schedule = pack(producers);

      if( set_proposed_producers( packed_schedule.data(),  packed_schedule.size() ) >= 0 ) {
         _gstate.last_producer_schedule_size = static_cast<decltype(_gstate.last_producer_schedule_size)>( top_producers.size() );
      }
   }

   void platform_contract::voteproducer( const account_name voter_name, const account_name producer_name, const asset vote_num ) {
      require_auth( voter_name );
      
      print(" voter_name=",name{voter_name}, " producer_name=",name{producer_name}, " vote_num =", vote_num.amount, "\n");
      besio_assert( vote_num.symbol == CORE_SYMBOL, "only support system token which has 4 precision" );
      besio_assert( vote_num.amount > 0 && vote_num.amount % 10000 == 0, "The number of votes must be an integer" );

      auto voter = _voters.find(voter_name);
      besio_assert( voter != _voters.end(), "user must stake before they can vote" ); /// staking creates voter object
      besio_assert( vote_num.amount <= voter->staked_balance.amount , "the balance available for the vote is insufficient" );

      auto prod = _producers.find(producer_name);
      besio_assert( prod != _producers.end(), "producer is not registered" );
      
      int64_t change_votes = 0; /** Increase or decrease voting num */
      
      votes_table votes_tbl( _self, voter_name );
      auto vts = votes_tbl.find( producer_name );
      if( vts == votes_tbl.end() ) {
         change_votes = vote_num.amount;
         votes_tbl.emplace( voter_name,[&]( vote_info & v ) {
            v.producer_name = producer_name;
            v.vote_num = vote_num;
            v.voteage = 0;
            v.voteage_update_time = current_time();
         });
      } else {
         change_votes = vote_num.amount - vts-> vote_num.amount;
         besio_assert( change_votes <= voter-> staked_balance.amount, "need votes change quantity < your staked balance" );

         votes_tbl.modify( vts, 0, [&]( vote_info & v ) {
            v.voteage += (v.vote_num.amount / 10000) * static_cast<int64_t>(( ( current_time() - v.voteage_update_time ) / voteage_basis ));
            v.voteage_update_time = current_time();
            v.vote_num = vote_num;
         });
      }

      _voters.modify( voter, 0, [&]( voter_info & v ) {
         v.staked_balance.amount -= change_votes;
      });
      
      
      _producers.modify( prod, 0, [&]( producer_info & p ) {
         p.total_voteage         += (p.total_votes/10000) * ( ( current_time() - p.voteage_update_time ) / voteage_basis );
         p.voteage_update_time   = current_time();
         p.total_votes           += change_votes;
      });

      _gstate.total_activated_votes += change_votes;
      if( _gstate.total_activated_votes >= min_activated_votes && _gstate.thresh_activated_votes_time == 0 ) {
         _gstate.thresh_activated_votes_time = current_time();
      }

  }
} /// namespace besioplatform
