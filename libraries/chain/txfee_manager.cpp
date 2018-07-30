/**
 *  @file
 *  @copyright defined in bes/LICENSE.txt
 */

#include <besio/chain/exceptions.hpp>
#include <besio/chain/txfee_manager.hpp>

namespace besio { namespace chain {

   txfee_manager::txfee_manager(){
        fee_map[N(newaccount)]      = asset(1000);
        fee_map[N(transfer)]        = asset(100);
        fee_map[N(vote)]            = asset(500);
        fee_map[N(unfreeze)]        = asset(100);
        fee_map[N(claim)]           = asset(300);
        fee_map[N(updatebp)]        = asset(100*10000);
        fee_map[N(setemergency)]    = asset(10*10000);

        fee_map[N(issue)]           = asset(100);
        fee_map[N(create)]          = asset(10*10000);
   }

   bool txfee_manager::check_transaction( const transaction& trx)const
   {
      for( const auto& act : trx.actions ) {
          for (const auto & perm : act.authorization) {
            if (perm.actor != trx.actions[0].authorization[0].actor) {
                return false;
            }
          }
      }
      return true;
   }

   asset txfee_manager::get_required_fee( const transaction& trx)const
   {
      auto fee = asset(0);
      for (const auto& act : trx.actions ) {
        auto it = fee_map.find(act.name);
        BES_ASSERT(it != fee_map.end(), action_validate_exception, "action name not include in feemap");
        fee += it->second;
      }

      return fee;
   }

} } /// namespace besio::chain
