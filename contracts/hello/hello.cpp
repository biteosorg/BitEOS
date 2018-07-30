#include <besiolib/besio.hpp>
using namespace besio;

class hello : public besio::contract {
  public:
      using contract::contract;

      /// @abi action 
      void hi( account_name user ) {
         print( "Hello, ", name{user} );
      }
};

BESIO_ABI( hello, (hi) )
