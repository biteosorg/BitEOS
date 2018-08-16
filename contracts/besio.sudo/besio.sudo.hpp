#pragma once

#include <besiolib/besio.hpp>

namespace besio {

   class sudo : public contract {
      public:
         sudo( account_name self ):contract(self){}

         void exec();

   };

} /// namespace besio
