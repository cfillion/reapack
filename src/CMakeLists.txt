add_library(reapack OBJECT
  about.cpp
  action.cpp
  api.cpp
  api_misc.cpp
  api_package.cpp
  api_repo.cpp
  archive.cpp
  archive_tasks.cpp
  browser.cpp
  browser_entry.cpp
  config.cpp
  control.cpp
  database.cpp
  dialog.cpp
  download.cpp
  event.cpp
  filedialog.cpp
  filesystem.cpp
  filter.cpp
  hash.cpp
  iconlist.cpp
  import.cpp
  index.cpp
  index_v1.cpp
  install.cpp
  listview.cpp
  main.cpp
  manager.cpp
  menu.cpp
  metadata.cpp
  obsquery.cpp
  package.cpp
  path.cpp
  platform.cpp
  progress.cpp
  reapack.cpp
  receipt.cpp
  registry.cpp
  remote.cpp
  report.cpp
  resource.rc
  serializer.cpp
  source.cpp
  string.cpp
  synchronize.cpp
  tabbar.cpp
  task.cpp
  thread.cpp
  time.cpp
  transaction.cpp
  version.cpp
  win32.cpp
)

if(APPLE)
  target_sources(reapack PRIVATE richedit.mm)
elseif(WIN32)
  target_sources(reapack PRIVATE richedit-win32.cpp)
else()
  target_sources(reapack PRIVATE richedit-gtk.cpp)
endif()

target_compile_features(reapack PUBLIC cxx_std_17)

target_compile_definitions(reapack PRIVATE
  REAPACK_FILE="$<TARGET_FILE_NAME:reaper_reapack>"
)

target_include_directories(reapack PRIVATE
  ${CMAKE_SOURCE_DIR}/vendor ${CMAKE_CURRENT_BINARY_DIR}
)

if(WIN32)
  target_compile_options(reapack PUBLIC /W3 /WX /wd4996)
  target_link_libraries(reapack Bcrypt Comctl32 Comdlg32 Gdi32 Shell32 User32)
else()
  target_compile_options(reapack PUBLIC
    -Wall -Wextra -Werror
    -Wno-unused-parameter -Wno-missing-field-initializers
    -Wno-unused-function -Wno-missing-braces
  )
endif()

find_package(Boost 1.56 REQUIRED)
find_package(CURL REQUIRED)
find_package(MiniZip REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(Threads REQUIRED)
find_package(TinyXML REQUIRED)
find_package(WDL REQUIRED)

target_link_libraries(reapack
  ${CMAKE_DL_LIBS} Boost::headers CURL::libcurl MiniZip::MiniZip SQLite::SQLite3
  Threads::Threads TinyXML::TinyXML WDL::WDL
)

if(NOT APPLE AND NOT WIN32)
  find_package(OpenSSL REQUIRED COMPONENTS Crypto)
  target_link_libraries(reapack OpenSSL::Crypto)
endif()

find_package(SWELL)
if(SWELL_FOUND)
  target_link_libraries(reapack SWELL::swell)

  find_package(PHP REQUIRED)

  # Transpile the Win32 resource file
  add_custom_command(
    OUTPUT  resource.rc_mac_dlg resource.rc_mac_menu
    COMMAND ${CMAKE_COMMAND} -E copy
      ${CMAKE_CURRENT_SOURCE_DIR}/resource.rc ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${PHP_EXECUTABLE} ${SWELL_RESGEN} resource.rc
    DEPENDS ${SWELL_RESGEN}
    MAIN_DEPENDENCY resource.rc
  )
endif()
