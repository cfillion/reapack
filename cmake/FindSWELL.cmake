if(WIN32 OR SWELL_FOUND)
  return()
endif()

find_package(WDL REQUIRED)

find_path(SWELL_INCLUDE_DIR
  NAMES swell/swell.h
  PATHS ${WDL_DIR}
  NO_DEFAULT_PATH
)
mark_as_advanced(SWELL_INCLUDE_DIR)

set(SWELL_DIR "${SWELL_INCLUDE_DIR}/swell")
set(SWELL_RESGEN "${SWELL_DIR}/mac_resgen.php")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SWELL REQUIRED_VARS SWELL_DIR SWELL_INCLUDE_DIR)

if(APPLE)
  add_library(swell
    ${SWELL_DIR}/swell.cpp
    ${SWELL_DIR}/swell-ini.cpp
    ${SWELL_DIR}/swell-miscdlg.mm
    ${SWELL_DIR}/swell-gdi.mm
    ${SWELL_DIR}/swell-kb.mm
    ${SWELL_DIR}/swell-menu.mm
    ${SWELL_DIR}/swell-misc.mm
    ${SWELL_DIR}/swell-dlg.mm
    ${SWELL_DIR}/swell-wnd.mm
  )

  target_compile_definitions(swell PRIVATE SWELL_APP_PREFIX=SWELL_REAPACK)

  find_library(COCOA Cocoa)
  find_library(CARBON Carbon)
  target_link_libraries(swell PUBLIC ${COCOA} ${CARBON})
else()
  add_library(swell ${SWELL_DIR}/swell-modstub-generic.cpp)

  target_compile_definitions(swell PUBLIC SWELL_PROVIDED_BY_APP)
endif()

target_include_directories(swell INTERFACE ${SWELL_INCLUDE_DIR})

add_library(SWELL::swell ALIAS swell)
