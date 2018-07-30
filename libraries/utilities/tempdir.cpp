/**
 *  @file
 *  @copyright defined in bes/LICENSE.txt
 */

#include <besio/utilities/tempdir.hpp>

#include <cstdlib>

namespace besio { namespace utilities {

fc::path temp_directory_path()
{
   const char* bes_tempdir = getenv("BES_TEMPDIR");
   if( bes_tempdir != nullptr )
      return fc::path( bes_tempdir );
   return fc::temp_directory_path() / "bes-tmp";
}

} } // besio::utilities
