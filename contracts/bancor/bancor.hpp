/**
 *  @file
 *  @copyright defined in bes/LICENSE.txt
 */
#pragma once

#include <besiolib/besio.hpp>
#include <besiolib/token.hpp>
#include <besiolib/reflect.hpp>
#include <besiolib/generic_currency.hpp>

#include <bancor/converter.hpp>
#include <currency/currency.hpp>

namespace bancor {
   typedef besio::generic_currency< besio::token<N(other),S(4,OTHER)> >  other_currency;
   typedef besio::generic_currency< besio::token<N(bancor),S(4,RELAY)> > relay_currency;
   typedef besio::generic_currency< besio::token<N(currency),S(4,CUR)> > cur_currency;

   typedef converter<relay_currency, other_currency, cur_currency > example_converter;
} /// bancor

