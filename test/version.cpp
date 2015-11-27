#include <catch.hpp>

#include <errors.hpp>
#include <version.hpp>

#include <string>

using namespace std;

static const char *M = "[database]";
TEST_CASE("invalid") {
  try {
    Version ver("hello");
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "invalid version name");
  }
}

TEST_CASE("major minor patch") {
  Version ver("1.2.3");
  REQUIRE(ver.name() == "1.2.3");
  REQUIRE(ver.code() == 1002003);
}

TEST_CASE("major minor") {
  Version ver("1.2");
  REQUIRE(ver.name() == "1.2");
  REQUIRE(ver.code() == 1002000);
}

TEST_CASE("major") {
  Version ver("1");
  REQUIRE(ver.name() == "1");
  REQUIRE(ver.code() == 1000000);
}

TEST_CASE("string suffix") {
  Version ver("1.2pre3");
  REQUIRE(ver.name() == "1.2pre3");
  REQUIRE(ver.code() == 1002003);
}

TEST_CASE("extra integer") {
  Version ver("1.2.3.4");
  REQUIRE(ver.name() == "1.2.3.4");
  REQUIRE(ver.code() == 1002003004);
}
