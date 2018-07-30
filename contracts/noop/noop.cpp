/**
 *  @file
 *  @copyright defined in bes/LICENSE.txt
 */

#include <besiolib/besio.hpp>

namespace besio {

   class noop: public contract {
      public:
         noop( account_name self ): contract( self ) { }
         void anyaction( account_name from,
                         const std::string& /*type*/,
                         const std::string& /*data*/ )
         {
            require_auth( from );
         }
   };

   BESIO_ABI( noop, ( anyaction ) )

} /// besio     
