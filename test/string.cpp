#include "helper.hpp"

#include <string.hpp>

static const char *M = "[string]";

TEST_CASE("string format", M) {
  const std::string &formatted = String::format("%d%% Hello %s!", 100, "World");
  CHECK(formatted.size() == 17);
  REQUIRE(formatted == "100% Hello World!");
}

TEST_CASE("indent string", M) {
  std::string actual;

  SECTION("simple")
    actual = String::indent("line1\nline2");

  SECTION("already indented")
    actual = String::indent("  line1\n  line2");

  REQUIRE(actual == "  line1\r\n  line2");
}

TEST_CASE("pretty-print numbers", M) {
  REQUIRE(String::number(42'000'000) == "42,000,000");
}
