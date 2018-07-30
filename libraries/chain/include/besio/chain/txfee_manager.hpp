/**
 *  @file
 *  @copyright defined in bes/LICENSE.txt
 */
#pragma once

#include <besio/chain/types.hpp>
#include <besio/chain/asset.hpp>

namespace besio { namespace chain {

   class controller;

   class txfee_manager {
      public:

         explicit txfee_manager();

         bool check_transaction( const transaction& trx)const;

         asset get_required_fee( const transaction& trx)const;


      private:
        std::map<action_name, asset> fee_map;
   };
   class fee_paramter {
     public:
       account_name name;
       asset fee;
       account_name producer;
       fee_paramter(account_name name, asset fee, account_name producer) : name(name), fee(fee), producer(producer) {};
    };
} } /// namespace besio::chain

FC_REFLECT(besio::chain::fee_paramter, (name)(fee)(producer))
