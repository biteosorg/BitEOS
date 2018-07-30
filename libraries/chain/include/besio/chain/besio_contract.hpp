/**
 *  @file
 *  @copyright defined in bes/LICENSE.txt
 */
#pragma once

#include <besio/chain/types.hpp>
#include <besio/chain/contract_types.hpp>

namespace besio { namespace chain {

   class apply_context;

   /**
    * @defgroup native_action_handlers Native Action Handlers
    */
   ///@{
   void apply_besio_newaccount(apply_context&);
   void apply_besio_updateauth(apply_context&);
   void apply_besio_deleteauth(apply_context&);
   void apply_besio_linkauth(apply_context&);
   void apply_besio_unlinkauth(apply_context&);

   /*
   void apply_besio_postrecovery(apply_context&);
   void apply_besio_passrecovery(apply_context&);
   void apply_besio_vetorecovery(apply_context&);
   */

   void apply_besio_setcode(apply_context&);
   void apply_besio_setabi(apply_context&);

   void apply_besio_canceldelay(apply_context&);
   ///@}  end action handlers

} } /// namespace besio::chain
