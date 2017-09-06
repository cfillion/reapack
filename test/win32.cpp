#include "helper.hpp"

#include <win32.hpp>

using namespace std;

static const char *M = "[win32]";

TEST_CASE("widen string", M) {
  SECTION("ascii") {
    const auto &wide = Win32::widen("hello world");
    REQUIRE(wide == L("hello world"));
    REQUIRE(wide.size() == 11);
  }

  SECTION("cyrillic") {
    const auto &wide = Win32::widen("世界");
    REQUIRE(wide == L("\u4e16\u754c"));
#ifdef _WIN32
    REQUIRE(wide.size() == 2);
#else
    REQUIRE(wide.size() == 6);
#endif
  }
}

TEST_CASE("narrow string", M) {
  SECTION("ascii") {
    const auto &narrow = Win32::narrow(L("hello world"));
    REQUIRE(narrow == "hello world");
  }

  SECTION("cyrillic") {
    const auto &narrow = Win32::narrow(L("\u4e16\u754c"));
    REQUIRE(narrow == "世界");
  }
}
