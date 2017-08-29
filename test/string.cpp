#include <catch.hpp>

#include <string.hpp>

using namespace std;

static const char *M = "[string]";

TEST_CASE("to_(w)string wrapper", M) {
  REQUIRE(String::from(42) == AUTOSTR("42"));
}

TEST_CASE("string to wstring to string", M) {
  SECTION("ascii")
    REQUIRE(String("hello world").toUtf8() == "hello world");

  SECTION("cyrillic")
    REQUIRE(String("Новая папка") == "Новая папка");
}

TEST_CASE("lengthof(array)", M) {
  struct { char m[2]; } arr[10];
  CHECK(sizeof(arr) == 20);
  REQUIRE(lengthof(arr) == 10);
}
