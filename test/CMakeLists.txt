add_executable(tests EXCLUDE_FROM_ALL
  action.cpp
  api.cpp
  database.cpp
  event.cpp
  filesystem.cpp
  filter.cpp
  hash.cpp
  helper.cpp
  helper.hpp
  index.cpp
  index_v1.cpp
  main.cpp
  metadata.cpp
  package.cpp
  path.cpp
  platform.cpp
  receipt.cpp
  registry.cpp
  remote.cpp
  serializer.cpp
  source.cpp
  string.cpp
  time.cpp
  version.cpp
  win32.cpp
)

# std::uncaught_exceptions is unavailable prior to macOS 10.12
target_compile_definitions(tests PRIVATE CATCH_CONFIG_NO_CPP17_UNCAUGHT_EXCEPTIONS)

target_include_directories(tests PRIVATE
  ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/vendor
)

find_package(Catch2 REQUIRED)
mark_as_advanced(Catch2_DIR)

target_link_libraries(tests Catch2::Catch2 reapack)
