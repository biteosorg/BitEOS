/**
 *  @file
 *  @copyright defined in bes/LICENSE.txt
 */
#include <besiolib/besio.hpp>

#include "test_api.hpp"

void test_types::types_size() {

   besio_assert( sizeof(int64_t) == 8, "int64_t size != 8");
   besio_assert( sizeof(uint64_t) ==  8, "uint64_t size != 8");
   besio_assert( sizeof(uint32_t) ==  4, "uint32_t size != 4");
   besio_assert( sizeof(int32_t) ==  4, "int32_t size != 4");
   besio_assert( sizeof(uint128_t) == 16, "uint128_t size != 16");
   besio_assert( sizeof(int128_t) == 16, "int128_t size != 16");
   besio_assert( sizeof(uint8_t) ==  1, "uint8_t size != 1");

   besio_assert( sizeof(account_name) ==  8, "account_name size !=  8");
   besio_assert( sizeof(table_name) ==  8, "table_name size !=  8");
   besio_assert( sizeof(time) ==  4, "time size !=  4");
   besio_assert( sizeof(besio::key256) == 32, "key256 size != 32" );
}

void test_types::char_to_symbol() {

   besio_assert( besio::char_to_symbol('1') ==  1, "besio::char_to_symbol('1') !=  1");
   besio_assert( besio::char_to_symbol('2') ==  2, "besio::char_to_symbol('2') !=  2");
   besio_assert( besio::char_to_symbol('3') ==  3, "besio::char_to_symbol('3') !=  3");
   besio_assert( besio::char_to_symbol('4') ==  4, "besio::char_to_symbol('4') !=  4");
   besio_assert( besio::char_to_symbol('5') ==  5, "besio::char_to_symbol('5') !=  5");
   besio_assert( besio::char_to_symbol('a') ==  6, "besio::char_to_symbol('a') !=  6");
   besio_assert( besio::char_to_symbol('b') ==  7, "besio::char_to_symbol('b') !=  7");
   besio_assert( besio::char_to_symbol('c') ==  8, "besio::char_to_symbol('c') !=  8");
   besio_assert( besio::char_to_symbol('d') ==  9, "besio::char_to_symbol('d') !=  9");
   besio_assert( besio::char_to_symbol('e') == 10, "besio::char_to_symbol('e') != 10");
   besio_assert( besio::char_to_symbol('f') == 11, "besio::char_to_symbol('f') != 11");
   besio_assert( besio::char_to_symbol('g') == 12, "besio::char_to_symbol('g') != 12");
   besio_assert( besio::char_to_symbol('h') == 13, "besio::char_to_symbol('h') != 13");
   besio_assert( besio::char_to_symbol('i') == 14, "besio::char_to_symbol('i') != 14");
   besio_assert( besio::char_to_symbol('j') == 15, "besio::char_to_symbol('j') != 15");
   besio_assert( besio::char_to_symbol('k') == 16, "besio::char_to_symbol('k') != 16");
   besio_assert( besio::char_to_symbol('l') == 17, "besio::char_to_symbol('l') != 17");
   besio_assert( besio::char_to_symbol('m') == 18, "besio::char_to_symbol('m') != 18");
   besio_assert( besio::char_to_symbol('n') == 19, "besio::char_to_symbol('n') != 19");
   besio_assert( besio::char_to_symbol('o') == 20, "besio::char_to_symbol('o') != 20");
   besio_assert( besio::char_to_symbol('p') == 21, "besio::char_to_symbol('p') != 21");
   besio_assert( besio::char_to_symbol('q') == 22, "besio::char_to_symbol('q') != 22");
   besio_assert( besio::char_to_symbol('r') == 23, "besio::char_to_symbol('r') != 23");
   besio_assert( besio::char_to_symbol('s') == 24, "besio::char_to_symbol('s') != 24");
   besio_assert( besio::char_to_symbol('t') == 25, "besio::char_to_symbol('t') != 25");
   besio_assert( besio::char_to_symbol('u') == 26, "besio::char_to_symbol('u') != 26");
   besio_assert( besio::char_to_symbol('v') == 27, "besio::char_to_symbol('v') != 27");
   besio_assert( besio::char_to_symbol('w') == 28, "besio::char_to_symbol('w') != 28");
   besio_assert( besio::char_to_symbol('x') == 29, "besio::char_to_symbol('x') != 29");
   besio_assert( besio::char_to_symbol('y') == 30, "besio::char_to_symbol('y') != 30");
   besio_assert( besio::char_to_symbol('z') == 31, "besio::char_to_symbol('z') != 31");

   for(unsigned char i = 0; i<255; i++) {
      if((i >= 'a' && i <= 'z') || (i >= '1' || i <= '5')) continue;
      besio_assert( besio::char_to_symbol((char)i) == 0, "besio::char_to_symbol() != 0");
   }
}

void test_types::string_to_name() {

   besio_assert( besio::string_to_name("a") == N(a) , "besio::string_to_name(a)" );
   besio_assert( besio::string_to_name("ba") == N(ba) , "besio::string_to_name(ba)" );
   besio_assert( besio::string_to_name("cba") == N(cba) , "besio::string_to_name(cba)" );
   besio_assert( besio::string_to_name("dcba") == N(dcba) , "besio::string_to_name(dcba)" );
   besio_assert( besio::string_to_name("edcba") == N(edcba) , "besio::string_to_name(edcba)" );
   besio_assert( besio::string_to_name("fedcba") == N(fedcba) , "besio::string_to_name(fedcba)" );
   besio_assert( besio::string_to_name("gfedcba") == N(gfedcba) , "besio::string_to_name(gfedcba)" );
   besio_assert( besio::string_to_name("hgfedcba") == N(hgfedcba) , "besio::string_to_name(hgfedcba)" );
   besio_assert( besio::string_to_name("ihgfedcba") == N(ihgfedcba) , "besio::string_to_name(ihgfedcba)" );
   besio_assert( besio::string_to_name("jihgfedcba") == N(jihgfedcba) , "besio::string_to_name(jihgfedcba)" );
   besio_assert( besio::string_to_name("kjihgfedcba") == N(kjihgfedcba) , "besio::string_to_name(kjihgfedcba)" );
   besio_assert( besio::string_to_name("lkjihgfedcba") == N(lkjihgfedcba) , "besio::string_to_name(lkjihgfedcba)" );
   besio_assert( besio::string_to_name("mlkjihgfedcba") == N(mlkjihgfedcba) , "besio::string_to_name(mlkjihgfedcba)" );
   besio_assert( besio::string_to_name("mlkjihgfedcba1") == N(mlkjihgfedcba2) , "besio::string_to_name(mlkjihgfedcba2)" );
   besio_assert( besio::string_to_name("mlkjihgfedcba55") == N(mlkjihgfedcba14) , "besio::string_to_name(mlkjihgfedcba14)" );

   besio_assert( besio::string_to_name("azAA34") == N(azBB34) , "besio::string_to_name N(azBB34)" );
   besio_assert( besio::string_to_name("AZaz12Bc34") == N(AZaz12Bc34) , "besio::string_to_name AZaz12Bc34" );
   besio_assert( besio::string_to_name("AAAAAAAAAAAAAAA") == besio::string_to_name("BBBBBBBBBBBBBDDDDDFFFGG") , "besio::string_to_name BBBBBBBBBBBBBDDDDDFFFGG" );
}

void test_types::name_class() {

   besio_assert( besio::name{besio::string_to_name("azAA34")}.value == N(azAA34), "besio::name != N(azAA34)" );
   besio_assert( besio::name{besio::string_to_name("AABBCC")}.value == 0, "besio::name != N(0)" );
   besio_assert( besio::name{besio::string_to_name("AA11")}.value == N(AA11), "besio::name != N(AA11)" );
   besio_assert( besio::name{besio::string_to_name("11AA")}.value == N(11), "besio::name != N(11)" );
   besio_assert( besio::name{besio::string_to_name("22BBCCXXAA")}.value == N(22), "besio::name != N(22)" );
   besio_assert( besio::name{besio::string_to_name("AAAbbcccdd")} == besio::name{besio::string_to_name("AAAbbcccdd")}, "besio::name == besio::name" );

   uint64_t tmp = besio::name{besio::string_to_name("11bbcccdd")};
   besio_assert(N(11bbcccdd) == tmp, "N(11bbcccdd) == tmp");
}
