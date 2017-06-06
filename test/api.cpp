#include <catch.hpp>

#include "helper/io.hpp"

#include <cstring>

#include <api.hpp>

using namespace std;

static const char *M = "[api]";

TEST_CASE("CompareVersions", M) {
  const auto CompareVersions = (int (*)(const char *, const char *,
    char *, int))API::CompareVersions.cImpl;

  char error[255] = {};

  SECTION("equal") {
    REQUIRE(CompareVersions("1.0", "1.0", error, sizeof(error)) == 0);
    REQUIRE(strcmp(error, "") == 0);
  }

  SECTION("lower") {
    REQUIRE(CompareVersions("1.0", "2.0", error, sizeof(error)) < 0);
    REQUIRE(strcmp(error, "") == 0);
  }

  SECTION("higher") {
    REQUIRE(CompareVersions("2.0", "1.0", error, sizeof(error)) > 0);
    REQUIRE(strcmp(error, "") == 0);
  }

  SECTION("invalid") {
    REQUIRE(CompareVersions("abc", "def", error, sizeof(error)) == 0);
    REQUIRE(strcmp(error, "invalid version name 'abc'") == 0);
  }

  SECTION("invalid no error buffer")
    CompareVersions("abc", "def", nullptr, 0); // no crash
}
