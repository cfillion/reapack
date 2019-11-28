if(MiniZip_FOUND)
  return()
endif()

find_package(WDL REQUIRED)
find_path(MiniZip_INCLUDE_DIR
  NAMES unzip.h
  PATHS ${WDL_DIR}
  PATH_SUFFIXES zlib
  NO_DEFAULT_PATH
)
mark_as_advanced(MiniZip_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MiniZip REQUIRED_VARS MiniZip_INCLUDE_DIR)

add_library(minizip
  ${MiniZip_INCLUDE_DIR}/ioapi.c
  ${MiniZip_INCLUDE_DIR}/unzip.c
  ${MiniZip_INCLUDE_DIR}/zip.c
)

target_include_directories(minizip INTERFACE ${MiniZip_INCLUDE_DIR})

find_package(ZLIB REQUIRED)
target_link_libraries(minizip ZLIB::ZLIB)

add_library(MiniZip::MiniZip ALIAS minizip)
