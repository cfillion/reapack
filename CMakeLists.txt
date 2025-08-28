cmake_minimum_required(VERSION 3.15)

option(RUNTIME_OPENSSL
  "Load OpenSSL at runtime instead of linking against a specific version" OFF)

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "")
endif()

if(DEFINED ENV{VCPKG_DEFAULT_TRIPLET} AND NOT DEFINED VCPKG_TARGET_TRIPLET)
  set(VCPKG_TARGET_TRIPLET "$ENV{VCPKG_DEFAULT_TRIPLET}" CACHE STRING "")
endif()

project(reapack LANGUAGES C CXX)
if(MSVC)
  add_compile_options(/utf-8)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  if(WIN32)
    set(USER_CONFIG_DIR "$ENV{APPDATA}")
  elseif(APPLE)
    set(USER_CONFIG_DIR "~/Library/Application Support")
  else()
    set(USER_CONFIG_DIR "~/.config")
  endif()

  set(CMAKE_INSTALL_PREFIX "${USER_CONFIG_DIR}/REAPER" CACHE PATH
    "REAPER resource path where to install ReaPack" FORCE)
endif()

if(CMAKE_OSX_ARCHITECTURES)
  list(JOIN CMAKE_OSX_ARCHITECTURES "-" ARCH_NAME)
elseif(MSVC_CXX_ARCHITECTURE_ID)
  set(ARCH_NAME ${MSVC_CXX_ARCHITECTURE_ID})
else()
  set(ARCH_NAME ${CMAKE_SYSTEM_PROCESSOR})
endif()

STRING(TOLOWER "${ARCH_NAME}" ARCH_NAME)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET "hidden")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL     ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE        ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)

if(VCPKG_TOOLCHAIN)
  set(CMAKE_MAP_IMPORTED_CONFIG_MINSIZEREL Release)
  set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release)
endif()

if(WIN32)
  foreach(arg
    CMAKE_C_FLAGS_DEBUG CMAKE_CXX_FLAGS_DEBUG
    CMAKE_C_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELEASE
    CMAKE_C_FLAGS_RELWITHDEBINFO CMAKE_CXX_FLAGS_RELWITHDEBINFO
    CMAKE_C_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_MINSIZEREL
  )
    # Embed debug information in the object files
    string(REGEX REPLACE "/Z[iI]" "/Z7" ${arg} "${${arg}}")
  endforeach()

  add_compile_options(
    # Exception handling model
    /EHsc

    # Enhance optimized debugging
    $<$<NOT:$<CONFIG:Debug>>:/Zo>

    # Eliminate duplicate strings
    $<$<NOT:$<CONFIG:Debug>>:/GF>

    # Enable function-level linking
    $<$<NOT:$<CONFIG:Debug>>:/Gy>

    # Remove unreferenced COMDAT
    $<$<NOT:$<CONFIG:Debug>>:/Zc:inline>
  )

  add_link_options(
    # Remove unreferenced code
    $<$<NOT:$<CONFIG:Debug>>:/OPT:REF>

    # Remove duplicate sections
    $<$<NOT:$<CONFIG:Debug>>:/OPT:ICF>

    # Use relative path to the PDB file to avoid exposing the full path
    /PDBALTPATH:%_PDB%
  )
else()
  add_compile_options(-fsigned-char -fstack-protector-strong -fdiagnostics-color)
endif()

find_package(Ruby)
if(RUBY_FOUND)
  add_custom_target(prepare
    COMMAND ruby prepare.rb src/*.{{h,c}pp,mm}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
endif()

add_library(reaper_reapack SHARED)
set_target_properties(reaper_reapack PROPERTIES
  PREFIX "" # disable the "lib" prefix
  OUTPUT_NAME "reaper_reapack-${ARCH_NAME}"
)
add_subdirectory(src)
target_link_libraries(reaper_reapack reapack)

# Add langpack target
add_executable(buildlangpack src/buildlangpack.cpp)
target_include_directories(buildlangpack PRIVATE ${CMAKE_SOURCE_DIR}/vendor)
target_link_libraries(buildlangpack WDL::WDL)

add_subdirectory(test)
add_custom_target(test
  COMMAND $<TARGET_FILE:tests>
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  USES_TERMINAL
)
add_dependencies(test tests)

set(REAPER_USER_PLUGINS "UserPlugins")
install(TARGETS reaper_reapack
  COMPONENT ReaPack
  RUNTIME DESTINATION "${REAPER_USER_PLUGINS}" # Windows .dll
  LIBRARY DESTINATION "${REAPER_USER_PLUGINS}" # Linux .so/macOS .dylib
)
