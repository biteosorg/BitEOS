#include <besiolib/besio.hpp>
#include <besiolib/print.hpp>
using namespace besio;

class payloadless : public besio::contract {
  public:
      using contract::contract;

      void doit() {
         print( "Im a payloadless action" );
      }
};

BESIO_ABI( payloadless, (doit) )
