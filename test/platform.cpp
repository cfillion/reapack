#include <catch.hpp>

#include <platform.hpp>

#include <utility>

using namespace std;

static const char *M = "[platform]";

TEST_CASE("default platform", M) {
  const Platform pl;
  REQUIRE(pl.value() == Platform::GenericPlatform);
  REQUIRE(pl.test());
}

TEST_CASE("platform from string", M) {
  SECTION("unknown")
    REQUIRE(Platform("hello") == Platform::UnknownPlatform);

  SECTION("generic")
    REQUIRE(Platform("all") == Platform::GenericPlatform);

  SECTION("generic windows")
    REQUIRE(Platform("windows") == Platform::WindowsPlatform);

  SECTION("windows 32-bit")
    REQUIRE(Platform("win32") == Platform::Win32Platform);

  SECTION("windows 64-bit")
    REQUIRE(Platform("win64") == Platform::Win64Platform);

  SECTION("generic os x")
    REQUIRE(Platform("darwin") == Platform::DarwinPlatform);

  SECTION("os x 32-bit")
    REQUIRE(Platform("darwin32") == Platform::Darwin32Platform);

  SECTION("os x 64-bit")
    REQUIRE(Platform("darwin64") == Platform::Darwin64Platform);
}

TEST_CASE("test platform", M) {
  const pair<Platform, bool> expected[] = {
    {Platform::GenericPlatform, true},

#ifdef __APPLE__
    {Platform::WindowsPlatform, false},
    {Platform::Win32Platform, false},
    {Platform::Win64Platform, false},

    {Platform::DarwinPlatform, true},
#ifdef __x86_64__
    {Platform::Darwin32Platform, false},
    {Platform::Darwin64Platform, true},
#else
    {Platform::Darwin32Platform, true},
    {Platform::Darwin64Platform, false},
#endif

#elif _WIN32
    {Platform::DarwinPlatform, false},
    {Platform::Darwin32Platform, false},
    {Platform::Darwin64Platform, false},

    {Platform::WindowsPlatform, true},
#ifdef _WIN64
    {Platform::Win32Platform, false},
    {Platform::Win64Platform, true},
#else
    {Platform::Win32Platform, true},
    {Platform::Win64Platform, false},
#endif
#endif
  };

  for(const auto &it : expected)
    REQUIRE(it.first.test() == it.second);
}
