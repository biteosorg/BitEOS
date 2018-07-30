/**
 *  @file
 *  @copyright defined in bes/LICENSE.txt
 */

#include <besiolib/besio.hpp>

namespace asserter {
   struct assertdef {
      int8_t      condition;
      std::string message;

      BESLIB_SERIALIZE( assertdef, (condition)(message) )
   };
}
