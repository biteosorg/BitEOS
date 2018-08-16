configure_file(${CMAKE_CURRENT_SOURCE_DIR}/core_symbol.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/core_symbol.hpp)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/globalconfig.hpp.in ${CMAKE_CURRENT_SOURCE_DIR}/globalconfig.hpp)

add_wast_library(TARGET besiolib
  INCLUDE_FOLDERS "${STANDARD_INCLUDE_FOLDERS}" ${CMAKE_SOURCE_DIR}/externals/magic_get/include
  DESTINATION_FOLDER ${CMAKE_CURRENT_BINARY_DIR}
)
