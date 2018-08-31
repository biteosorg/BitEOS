/**
 *  @file
 *  @copyright defined in bes/LICENSE.txt
 */
#pragma once

#include <besio.platform/native.hpp>
#include <besiolib/asset.hpp>
#include <besiolib/time.hpp>
#include <besiolib/privileged.hpp>
#include <besiolib/singleton.hpp>
#include <besio.platform/exchange_state.hpp>

#include <string>

namespace besioplatform {

   using besio::asset;
   using besio::indexed_by;
   using besio::const_mem_fun;
   using besio::block_timestamp;

   struct name_bid {
     account_name            newname;
     account_name            high_bidder;
     int64_t                 high_bid = 0; ///< negative high_bid == closed auction waiting to be claimed
     uint64_t                last_bid_time = 0;

     auto     primary_key()const { return newname;                          }
     uint64_t by_high_bid()const { return static_cast<uint64_t>(-high_bid); }
   };

   typedef besio::multi_index< N(namebids), name_bid,
                               indexed_by<N(highbid), const_mem_fun<name_bid, uint64_t, &name_bid::by_high_bid>  >
                               >  name_bid_table;

   struct user_resources {
      account_name  owner;
      asset         net_weight;
      asset         cpu_weight;
      int64_t       ram_bytes = 0;
      uint32_t      ram_trading_fee_rate = 0;
      uint64_t      ram_last_warning_time = 0;
      uint64_t      ram_info_last_update_time = 0;
      

      uint64_t primary_key()const { return owner; }

      // explicit serialization macro is not necessary, used here only to improve compilation time
      BESLIB_SERIALIZE( user_resources, (owner)(net_weight)(cpu_weight)(ram_bytes)(ram_trading_fee_rate)(ram_last_warning_time)(ram_info_last_update_time) )
   };
   

   struct besio_global_state : besio::blockchain_parameters {
      uint64_t free_ram()const { return max_ram_size - total_ram_bytes_reserved; }

      uint64_t             max_ram_size = 64ll*1024 * 1024 * 1024;
      uint64_t             total_ram_bytes_reserved = 0;
      int64_t              total_ram_stake = 0;

      block_timestamp      last_producer_schedule_update;
      uint64_t             last_rewards_bucket_fill = 0;
      int64_t              pervote_bucket = 0;
      int64_t              perblock_bucket = 0;
      uint32_t             total_unpaid_blocks = 0; /// all blocks which have been produced but not paid
      int64_t              total_activated_votes = 0;
      uint64_t             thresh_activated_votes_time = 0;
      uint16_t             last_producer_schedule_size = 0;
      block_timestamp      last_name_close;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      BESLIB_SERIALIZE_DERIVED( besio_global_state, besio::blockchain_parameters,
                                (max_ram_size)(total_ram_bytes_reserved)(total_ram_stake)
                                (last_producer_schedule_update)(last_rewards_bucket_fill)
                                (pervote_bucket)(perblock_bucket)(total_unpaid_blocks)(total_activated_votes)(thresh_activated_votes_time)
                                (last_producer_schedule_size)(last_name_close) )
   };

   struct producer_info {
      account_name          owner;
      double                total_votes = 0;
      besio::public_key     producer_key; /// a packed public key object
      bool                  is_active = true;
      std::string           url;
      uint32_t              unpaid_blocks = 0;
      uint32_t              commission_rate = 0;
      uint64_t              last_commission_rate_adjustment_time = 0;
      uint16_t              location = 0;
      int128_t              total_voteage;
      uint64_t              voteage_update_time = current_time();
      int64_t               rewards_producer_balance = 0;
      int64_t               rewards_voters_balance = 0;
      

      uint64_t primary_key()const { return owner;                                   }
      double   by_votes()const    { return is_active ? -total_votes : total_votes;  }
      bool     active()const      { return is_active;                               }
      void     deactivate()       { producer_key = public_key(); is_active = false; }

      // explicit serialization macro is not necessary, used here only to improve compilation time
      BESLIB_SERIALIZE( producer_info, (owner)(total_votes)(producer_key)(is_active)(url)
                        (unpaid_blocks)(commission_rate)(last_commission_rate_adjustment_time)(location)(total_voteage)(voteage_update_time)
                        (rewards_producer_balance)(rewards_voters_balance)
       )
   };
   struct voter_info {
      account_name         votername = 0; 
      besio::asset         staked_balance = asset(0, CORE_SYMBOL);
      uint64_t             last_claim_time = 0;
      uint64_t             primary_key() const { return votername; }

      BESLIB_SERIALIZE( voter_info, (votername)(staked_balance)(last_claim_time))
   };

   struct vote_info {
      account_name         producer_name = 0; 
      besio::asset         vote_num = asset(0, CORE_SYMBOL);
      uint64_t             voteage_update_time = current_time();
      int64_t              voteage = 0;

      uint64_t             primary_key() const { return producer_name; }

      BESLIB_SERIALIZE( vote_info, (producer_name)(vote_num)(voteage_update_time)(voteage) )
   };
  
   typedef besio::multi_index< N(voters), voter_info>  voters_table;
   
   typedef besio::multi_index< N(votes), vote_info>  votes_table;

   typedef besio::multi_index< N(producers), producer_info,
                               indexed_by<N(prototalvote), const_mem_fun<producer_info, double, &producer_info::by_votes>  >
                               >  producers_table;

   typedef besio::singleton<N(global), besio_global_state> global_state_singleton;

   //   static constexpr uint32_t     max_inflation_rate = 5;  // 5% annual inflation
   static constexpr uint32_t     seconds_per_day = 24 * 3600;
   static constexpr uint64_t     platform_token_symbol = CORE_SYMBOL;
   static constexpr uint64_t     min_commission_adjustment_period = 3600*24*7*1000000ll;    // 7days
   static constexpr uint64_t     distribute_rewards_preiod = 3600*24*3*1000000ll;           // 3days
   static constexpr uint64_t     claim_rewards_preiod  = 3600*24*7*1000000ll;               // 7days
   static constexpr uint64_t     voteage_basis         = claim_rewards_preiod / 10;  // claim rewards preiod 's one fifth
   static constexpr double       max_commission_adjustment_rate = 0.0005;            // 
   
   static constexpr uint64_t     ram_trade_punitive_fee_preiod =  3600*24*1000000ll;      // 1day
   static constexpr uint64_t     ram_trade_punitive_fee_warning_period = 7;          // 7 period's,7days
   static constexpr double       ram_punitive_unuse_rate = 0.5;                      // 50%
   static constexpr double       ram_trade_basis_fee_rate = 0.005;                   // 0.5%
   static constexpr double       ram_trade_max_fee_rate = 0.5;                       // 50%


   class platform_contract : public native {
      private:
         voters_table           _voters;
         producers_table        _producers;
         global_state_singleton _global;

         besio_global_state     _gstate;
         rammarket              _rammarket;

      public:
         platform_contract( account_name s );
         ~platform_contract();

         // Actions:
         void onblock( block_timestamp timestamp, account_name producer );
                      // const block_header& header ); /// only parse first 3 fields of block header

         // functions defined in delegate_bandwidth.cpp

         /**
          *  Stakes SYS from the balance of 'from' for the benfit of 'receiver'.
          *  If transfer == true, then 'receiver' can unstake to their account
          *  Else 'from' can unstake at any time.
          */
         void delegatebw( account_name from, account_name receiver,
                          asset stake_net_quantity, asset stake_cpu_quantity, bool transfer );


         /**
          *  Decreases the total tokens delegated by from to receiver and/or
          *  frees the memory associated with the delegation if there is nothing
          *  left to delegate.
          *
          *  This will cause an immediate reduction in net/cpu bandwidth of the
          *  receiver.
          *
          *  A transaction is scheduled to send the tokens back to 'from' after
          *  the staking period has passed. If existing transaction is scheduled, it
          *  will be canceled and a new transaction issued that has the combined
          *  undelegated amount.
          *
          *  The 'from' account loses voting power as a result of this call and
          *  all producer tallies are updated.
          */
         void undelegatebw( account_name from, account_name receiver,
                            asset unstake_net_quantity, asset unstake_cpu_quantity );


         /**
          * Increases receiver's ram quota based upon current price and quantity of
          * tokens provided. An inline transfer from receiver to platform contract of
          * tokens will be executed.
          */
         void buyram( account_name buyer, account_name receiver, asset tokens );
         void buyrambytes( account_name buyer, account_name receiver, uint32_t bytes );

         /**
          *  Reduces quota my bytes and then performs an inline transfer of tokens
          *  to receiver based upon the average purchase price of the original quota.
          */
         void sellram( account_name receiver, int64_t bytes );

         /**
          *  This action is called after the delegation-period to claim all pending
          *  unstaked tokens belonging to owner
          */
         void refund( account_name owner );

         // functions defined in voting.cpp

         void regproducer( const account_name producer, const public_key& producer_key, const std::string& url, uint16_t location, uint32_t commission_rate);

         void unregprod( const account_name producer );

         void setram( uint64_t max_ram_size );

         void voteproducer( const account_name voter, const account_name producer, const asset stake);

         void setparams( const besio::blockchain_parameters& params );

         // functions defined in producer_pay.cpp
         void claimrewards( const account_name owner , const account_name producer );

         void setpriv( account_name account, uint8_t ispriv );

         void rmvproducer( account_name producer );

         void bidname( account_name bidder, account_name newname, asset bid );

         void onramusage(account_name name);
      private:
         void update_elected_producers( block_timestamp timestamp );

         // Implementation details:

         //defind in delegate_bandwidth.cpp
         void changebw( account_name from, account_name receiver,
                        asset stake_net_quantity, asset stake_cpu_quantity, bool transfer );
				 
         double cal_ram_real_time_trad_fee_rate(user_resources & resobj);

         double get_ram_trad_fee_rate(account_name account);

         //defined in voting.hpp
         static besio_global_state get_default_parameters();

         //defined in producer_pay.cpp
         void distribute_rewards(const uint64_t distribut_time);
		 
         void distribute_rewards_task();
         
   };

} /// besioplatform
