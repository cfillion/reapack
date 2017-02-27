#include <catch.hpp>

#include <encoding.hpp>

using namespace std;

static const char *M = "[encoding]";

TEST_CASE("to_autostring", M) {
  REQUIRE(to_autostring(42) == AUTO_STR("42"));
}

TEST_CASE("string to wstring to string", M) {
  SECTION("ascii") {
    const auto_string &wstr = make_autostring("hello world");
    const string &str = from_autostring(wstr);

    REQUIRE(str == "hello world");
  }

  SECTION("cyrillic") {
    const auto_string &wstr = make_autostring("Новая папка");
    const string &str = from_autostring(wstr);

    REQUIRE(str == "Новая папка");
  }
}

TEST_CASE("auto_size", M) {
  auto_char test[42] = {};
  REQUIRE(auto_size(test) == 42);
}
