if(TinyXML_FOUND)
  return()
endif()

find_package(WDL REQUIRED)

find_path(TinyXML_INCLUDE_DIR
  NAMES tinyxml.h
  PATHS ${WDL_DIR}
  PATH_SUFFIXES tinyxml
  NO_DEFAULT_PATH
)
mark_as_advanced(TinyXML_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(TinyXML REQUIRED_VARS TinyXML_INCLUDE_DIR)

add_library(tinyxml
  ${TinyXML_INCLUDE_DIR}/tinyxml.cpp ${TinyXML_INCLUDE_DIR}/tinystr.cpp
  ${TinyXML_INCLUDE_DIR}/tinyxmlparser.cpp ${TinyXML_INCLUDE_DIR}/tinyxmlerror.cpp
)

target_include_directories(tinyxml INTERFACE ${TinyXML_INCLUDE_DIR})

add_library(TinyXML::TinyXML ALIAS tinyxml)
