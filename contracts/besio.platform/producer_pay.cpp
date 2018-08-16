#include "besio.platform.hpp"

#include <besio.token/besio.token.hpp>

namespace besioplatform {

   const int64_t  min_pervote_daily_pay = 100'0000;
   const int64_t  min_activated_votes   = 15'000'000'000'0000;
   const double   continuous_rate       = 0.01;                     // 1% annual rate
   const double   perblock_rate         = 0.50;                     // 50%
   
   const uint32_t blocks_per_year       = 52*7*24*2*3600;           // half seconds per year
   const uint32_t seconds_per_year      = 52*7*24*3600;
   const uint32_t blocks_per_day        = 2 * 24 * 3600;
   const uint32_t blocks_per_hour       = 2 * 3600;
   const uint64_t useconds_per_day      = 24 * 3600 * uint64_t(1000000);
   const uint64_t useconds_per_year     = seconds_per_year*1000000ll;


   void platform_contract::onblock( block_timestamp timestamp, account_name producer ) {
      using namespace besio;

      require_auth(N(besio));

      /** until activated stake crosses this threshold no new rewards are paid */
      if( _gstate.total_activated_votes < min_activated_votes )
         return;

      if( _gstate.last_rewards_bucket_fill == 0 )  /// start the presses
         _gstate.last_rewards_bucket_fill = current_time();


      /**
       * At startup the initial producer may not be one that is registered / elected
       * and therefore there may be no producer object for them.
       */
      auto prod = _producers.find(producer);
      if ( prod != _producers.end() ) {
         _gstate.total_unpaid_blocks++;
         _producers.modify( prod, 0, [&](auto& p ) {
               p.unpaid_blocks++;
         });
      }
      

      /// only update block producers once every minute, block_timestamp is in half seconds
      if( timestamp.slot - _gstate.last_producer_schedule_update.slot > 120 ) {
         distribute_rewards_task();

         update_elected_producers( timestamp );
         
         if( (timestamp.slot - _gstate.last_name_close.slot) > blocks_per_day ) {
            name_bid_table bids(_self,_self);
            auto idx = bids.get_index<N(highbid)>();
            auto highest = idx.begin();
            if( highest != idx.end() &&
                highest->high_bid > 0 &&
                highest->last_bid_time < (current_time() - useconds_per_day) &&
                _gstate.thresh_activated_votes_time > 0 &&
                (current_time() - _gstate.thresh_activated_votes_time) > 14 * useconds_per_day ) {
                   _gstate.last_name_close = timestamp;
                   idx.modify( highest, 0, [&]( auto& b ){
                         b.high_bid = -b.high_bid;
               });
            }
         }
      }
   }

   using namespace besio;
   void platform_contract::claimrewards( const account_name owner, const account_name producer ) {
       require_auth(owner);
       const auto& prod = _producers.get( producer, "producer not found" );
       const auto& voter = _voters.get( owner, "voter not found" );
       
       auto ct = current_time();
       besio_assert( _gstate.total_activated_votes >= min_activated_votes, "not enough has been staked for producers to claim rewards" );
       besio_assert( (ct - voter.last_claim_time) >= claim_rewards_preiod, "already claimed rewards within past preiod" );

       if( owner == producer ){
          print("get prpducer rewards, producer is ", name{owner}, "\n");
          if( prod.rewards_producer_balance > 0 ) {
            INLINE_ACTION_SENDER(besio::token, transfer)( N(besio.token), {N(besio.rpay),N(active)},
                                                         { N(besio.rpay), owner, asset(prod.rewards_producer_balance), std::string("producer get claim") } );
            _producers.modify( prod, 0, [&](auto& p) {
                p.rewards_producer_balance = 0;
            });

            _voters.modify( voter, 0, [&](auto& v) {
                v.last_claim_time = ct;
            });
         }
       }else{
           votes_table votes_tbl( _self, owner );
           const auto & vts = votes_tbl.get( producer, "voter have not add votes to the the producer yet" );
           
           int128_t newest_voteage = static_cast<int128_t>( vts.voteage + (vts.vote_num.amount / 10000) * static_cast<int64_t>( ( current_time() - vts.voteage_update_time ) / voteage_basis ) );
           int128_t newest_total_voteage = static_cast<int128_t>( prod.total_voteage + (prod.total_votes/10000) * ( ( current_time() - prod.voteage_update_time ) / voteage_basis ));
           besio_assert( newest_total_voteage > 0, "claim is not available yet" );
           
           int128_t amount_voteage = (int128_t)prod.rewards_voters_balance * (int128_t)newest_voteage;
           int64_t reward = static_cast<int64_t>( (int128_t)amount_voteage / (int128_t)newest_total_voteage );
           besio_assert( 0 <= reward && reward <= prod.rewards_voters_balance, "rewards don't count" );
           
           INLINE_ACTION_SENDER(besio::token, transfer)( N(besio.token), {N(besio.rpay),N(active)},
                                                         { N(besio.rpay), owner, asset(reward), std::string("voter get claim") } );

           _voters.modify( voter, 0, [&](auto& v) {
              v.last_claim_time = ct;
           });

           votes_tbl.modify( vts, 0, [&]( vote_info & v ) {
              v.voteage = 0;
              v.voteage_update_time = current_time();
           });

           _producers.modify( prod, 0, [&]( producer_info & p ) {
              p.rewards_voters_balance -= reward;
              p.total_voteage = newest_total_voteage - newest_voteage;
              p.voteage_update_time = current_time();
           });
       }
   }
   
   void platform_contract::distribute_rewards_task() {

       auto ct = current_time();
       const auto usecs_since_last_distributed = ct - _gstate.last_rewards_bucket_fill;
       if(usecs_since_last_distributed >= distribute_rewards_preiod && _gstate.last_rewards_bucket_fill > 0 ){
           distribute_rewards(ct);
       }
   }

   void platform_contract::distribute_rewards(const uint64_t distribut_time) {
       besio_assert(_gstate.total_activated_votes >= min_activated_votes,
                          "cannot distributing rewards until the chain is activated (at least 15% of all tokens participate in voting)");
       
       const auto usecs_since_last_distributed = distribut_time - _gstate.last_rewards_bucket_fill;
       
       if ( _gstate.last_rewards_bucket_fill > 0 ){
           const asset token_supply = token(N(besio.token)).get_supply(symbol_type(platform_token_symbol).name());
           
           print("token_supply is:", token_supply.amount, token_supply.symbol, "\n");
           auto new_tokens = static_cast<int64_t>((continuous_rate * double(token_supply.amount) * double(usecs_since_last_distributed)) / double(useconds_per_year));
           print("new_tokens is:", new_tokens, "\n");
           
           auto to_perblock_pay = static_cast<int64_t>(new_tokens * perblock_rate);
           print("to_perblock_pay = ", to_perblock_pay, "\n");

           auto to_standby_pay = new_tokens - to_perblock_pay ;
           print("to_standby_pay = ", to_standby_pay, "\n");
           
           INLINE_ACTION_SENDER(besio::token, transfer)( N(besio.token), {N(besio), N(active)},
                                                        {N(besio), N(besio.rpay), asset(new_tokens), "fund per-rewards bucket"});
           
           _gstate.perblock_bucket += to_perblock_pay;
           print("_gstate.perblock_bucket = ", _gstate.perblock_bucket, "\n");

           _gstate.pervote_bucket += to_standby_pay;
           print("_gstate.pervote_bucket = ", _gstate.pervote_bucket, "\n");
           _gstate.last_rewards_bucket_fill = distribut_time;
           print("_gstate.last_rewards_bucket_fill = ", _gstate.last_rewards_bucket_fill, "\n");
           print("_gstate.total_unpaid_blocks = ", _gstate.total_unpaid_blocks, "\n");
           print("------------------------------\n");
           
           int64_t pervote_bucket_used = 0;
           int64_t perblock_bucket_used = 0;
           int64_t total_unpaid_blocks_used = 0;
           int64_t total_votes_used = 0;
           for (auto & prod : _producers) {
               
               print("producer is : ", name{prod.owner}, "\n");
               if (false == prod.active()) {
                   continue;
               }
                     
               int64_t per_block_pay = 0;
               if (_gstate.total_unpaid_blocks > 0) {
                   per_block_pay = (_gstate.perblock_bucket * prod.unpaid_blocks) / _gstate.total_unpaid_blocks;
                   print("per_block_pay = ", per_block_pay, "\n");
                   print("prod.unpaid_blocks = ", prod.unpaid_blocks, "\n");
               }
               int64_t per_vote_pay = 0;
               per_vote_pay = int64_t((_gstate.pervote_bucket * prod.total_votes) / _gstate.total_activated_votes);
               print("per_vote_pay = ", per_vote_pay, "\n");
               print("prod.total_votes = ", prod.total_votes, "\n");
               
               pervote_bucket_used += per_vote_pay;
               perblock_bucket_used += per_block_pay;
               total_unpaid_blocks_used += prod.unpaid_blocks;
               total_votes_used += prod.total_votes;
               
               int64_t to_all_rewards = (per_block_pay + per_vote_pay);
               int64_t to_voters_reward  = static_cast<int64_t>( to_all_rewards * (double(prod.commission_rate)/10000));
               print("to_all_rewards = ", to_all_rewards, "\n");
               print("to_voters_reward = ", to_voters_reward, "\n");
               print("prod.commission_rate = ", prod.commission_rate, "\n");
               print("double(prod.commission_rate/10000) = ", double(prod.commission_rate)/10000, "\n");

               _producers.modify(prod, 0, [&](auto &p) {
                   p.unpaid_blocks = 0;
                   p.rewards_voters_balance += to_voters_reward;
                   p.rewards_producer_balance += (to_all_rewards - to_voters_reward);
                   print("p.rewards_producer_balance = ", p.rewards_producer_balance, "\n");
                   print("p.rewards_voters_balance = ", p.rewards_voters_balance, "\n");
                });
                print("------------------------------\n");   
            }

            _gstate.pervote_bucket -= pervote_bucket_used;
            print("_gstate.pervote_bucket = ", _gstate.pervote_bucket, "\n");
            _gstate.perblock_bucket -= perblock_bucket_used;
            print("_gstate.perblock_bucket = ", _gstate.perblock_bucket, "\n");
            _gstate.total_unpaid_blocks -= total_unpaid_blocks_used;
            print("_gstate.total_unpaid_blocks = ", _gstate.total_unpaid_blocks, "\n");
            print("_gstate.total_activated_votes = ", _gstate.total_activated_votes, "\n");
            print("total_votes_used = ", total_votes_used);
            
       }
   }

} //namespace besioplatform
