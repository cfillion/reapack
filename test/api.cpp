#include <catch.hpp>

#include "helper/io.hpp"

#include <api.hpp>

using namespace std;

static const char *M = "[api]";

TEST_CASE("CompareVersions", M) {
  auto CompareVersions = (bool (*)(const char *, const char *,
    int*, char *, int))API::CompareVersions.cImpl;

  int result;
  char error[255] = {};

  SECTION("equal") {
    REQUIRE(CompareVersions("1.0", "1.0", &result, error, sizeof(error)));
    REQUIRE(result == 0);
    REQUIRE(strcmp(error, "") == 0);
  }

  SECTION("lower") {
    REQUIRE(CompareVersions("1.0", "2.0", &result, error, sizeof(error)));
    REQUIRE(result < 0);
    REQUIRE(strcmp(error, "") == 0);
  }

  SECTION("higher") {
    REQUIRE(CompareVersions("2.0", "1.0", &result, error, sizeof(error)));
    REQUIRE(result > 0);
    REQUIRE(strcmp(error, "") == 0);
  }

  SECTION("invalid") {
    REQUIRE_FALSE(CompareVersions("abc", "def", &result, error, sizeof(error)));
    REQUIRE(result == 0);
    REQUIRE(strcmp(error, "invalid version name 'abc'") == 0);
  }

  SECTION("invalid no error buffer")
    REQUIRE_FALSE(CompareVersions("abc", "def", &result, nullptr, 0));
}
