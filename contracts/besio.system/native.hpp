/**
 *  @file
 *  @copyright defined in bes/LICENSE.txt
 */
#pragma once

#include <besiolib/action.hpp>
#include <besiolib/public_key.hpp>
#include <besiolib/types.hpp>
#include <besiolib/print.hpp>
#include <besiolib/privileged.h>
#include <besiolib/optional.hpp>
#include <besiolib/producer_schedule.hpp>
#include <besiolib/contract.hpp>

namespace besiosystem {
   using besio::permission_level;
   using besio::public_key;

   typedef std::vector<char> bytes;

   struct permission_level_weight {
      permission_level  permission;
      weight_type       weight;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      BESLIB_SERIALIZE( permission_level_weight, (permission)(weight) )
   };

   struct key_weight {
      public_key   key;
      weight_type  weight;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      BESLIB_SERIALIZE( key_weight, (key)(weight) )
   };

   struct authority {
      uint32_t                              threshold;
      uint32_t                              delay_sec;
      std::vector<key_weight>               keys;
      std::vector<permission_level_weight>  accounts;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      BESLIB_SERIALIZE( authority, (threshold)(delay_sec)(keys)(accounts) )
   };

   struct block_header {
      uint32_t                                  timestamp;
      account_name                              producer;
      uint16_t                                  confirmed = 0;
      block_id_type                             previous;
      checksum256                               transaction_mroot;
      checksum256                               action_mroot;
      uint32_t                                  schedule_version = 0;
      besio::optional<besio::producer_schedule> new_producers;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      BESLIB_SERIALIZE(block_header, (timestamp)(producer)(confirmed)(previous)(transaction_mroot)(action_mroot)
                                     (schedule_version)(new_producers))
   };


   /*
    * Method parameters commented out to prevent generation of code that parses input data.
    */
   class native : public besio::contract {
      public:

         using besio::contract::contract;

         /**
          *  Called after a new account is created. This code enforces resource-limits rules
          *  for new accounts as well as new account naming conventions.
          *
          *  1. accounts cannot contain '.' symbols which forces all acccounts to be 12
          *  characters long without '.' until a future account auction process is implemented
          *  which prevents name squatting.
          *
          *  2. new accounts must stake a minimal number of tokens (as set in system parameters)
          *     therefore, this method will execute an inline buyram from receiver for newacnt in
          *     an amount equal to the current new account creation fee.
          */
         void newaccount( account_name     creator,
                          account_name     newact
                          /*  no need to parse authorites
                          const authority& owner,
                          const authority& active*/ );


         void updateauth( /*account_name     account,
                                 permission_name  permission,
                                 permission_name  parent,
                                 const authority& data*/ ) {}

         void deleteauth( /*account_name account, permission_name permission*/ ) {}

         void linkauth( /*account_name    account,
                               account_name    code,
                               action_name     type,
                               permission_name requirement*/ ) {}

         void unlinkauth( /*account_name account,
                                 account_name code,
                                 action_name  type*/ ) {}

         void canceldelay( /*permission_level canceling_auth, transaction_id_type trx_id*/ ) {}

         void onerror( /*const bytes&*/ ) {}

   };
}
