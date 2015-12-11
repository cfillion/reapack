#include <catch.hpp>

#include <source.hpp>

#include <errors.hpp>

using namespace std;

static const char *M = "[source]";

TEST_CASE("convert platforms", M) {
  SECTION("unknown") {
    REQUIRE(Source::ConvertPlatform("hello") == Source::UnknownPlatform);
  }

  SECTION("generic") {
    REQUIRE(Source::ConvertPlatform("all") == Source::GenericPlatform);
  }

  SECTION("generic windows") {
    REQUIRE(Source::ConvertPlatform("windows") == Source::WindowsPlatform);
  }

  SECTION("windows 32-bit") {
    REQUIRE(Source::ConvertPlatform("win32") == Source::Win32Platform);
  }

  SECTION("windows 64-bit") {
    REQUIRE(Source::ConvertPlatform("win64") == Source::Win64Platform);
  }

  SECTION("generic os x") {
    REQUIRE(Source::ConvertPlatform("darwin") == Source::DarwinPlatform);
  }

  SECTION("os x 32-bit") {
    REQUIRE(Source::ConvertPlatform("darwin32") == Source::Darwin32Platform);
  }

  SECTION("os x 64-bit") {
    REQUIRE(Source::ConvertPlatform("darwin64") == Source::Darwin64Platform);
  }
}

TEST_CASE("empty source url", M) {
  try {
    Source source(Source::UnknownPlatform, string());
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "empty source url");
  }
}

