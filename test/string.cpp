#include "helper.hpp"

#include <string.hpp>

using namespace std;

static const char *M = "[string]";

TEST_CASE("to_string -> to_wstring alias", M) {
  REQUIRE(to_string(42) == L("42"));
}

TEST_CASE("string to wstring to string", M) {
  SECTION("ascii")
    REQUIRE(static_cast<string>(String("hello world")) == "hello world");

  SECTION("cyrillic")
    REQUIRE(static_cast<string>(String("Новая папка")) == "Новая папка");
}

TEST_CASE("lengthof(array)", M) {
  struct { char m[2]; } arr[10];
  CHECK(sizeof(arr) == 20);
  REQUIRE(lengthof(arr) == 10);
}

TEST_CASE("snprintf(char)") {
  char buf[10] = "abcdefghi";
  snprintf(buf, 5, "%s", "123456789");

  CHECK(string(buf) == "1234");
  CHECK(string(&buf[5]) == "fghi");
  REQUIRE(buf[4] == '\0');
}

#ifdef _WIN32
TEST_CASE("snprintf(wchar_t)") {
  wchar_t buf[10] = L"abcdefghi";
  snprintf(buf, 5, L"%s", L"123456789");

  CHECK(wstring(buf) == L"1234");
  CHECK(wstring(&buf[5]) == L"fghi");
  REQUIRE(buf[4] == L'\0');
}
#endif
