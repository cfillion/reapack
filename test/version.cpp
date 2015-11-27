#include <catch.hpp>

#include <errors.hpp>
#include <version.hpp>

#include <string>

using namespace std;

static const char *M = "[database]";

TEST_CASE("invalid", M) {
  try {
    Version ver("hello");
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "invalid version name");
  }
}

TEST_CASE("major minor patch version", M) {
  Version ver("1.2.3");
  REQUIRE(ver.name() == "1.2.3");
  REQUIRE(ver.code() == 1002003);
}

TEST_CASE("major minor version", M) {
  Version ver("1.2");
  REQUIRE(ver.name() == "1.2");
  REQUIRE(ver.code() == 1002000);
}

TEST_CASE("major version", M) {
  Version ver("1");
  REQUIRE(ver.name() == "1");
  REQUIRE(ver.code() == 1000000);
}

TEST_CASE("version with string suffix", M) {
  Version ver("1.2pre3");
  REQUIRE(ver.name() == "1.2pre3");
  REQUIRE(ver.code() == 1002003);
}

TEST_CASE("version with extra integer", M) {
  Version ver("1.2.3.4");
  REQUIRE(ver.name() == "1.2.3.4");
  REQUIRE(ver.code() == 1002003004);
}

TEST_CASE("convert platforms", M) {
  SECTION("unknown") {
    REQUIRE(Source::convertPlatform("hello") == Source::UnknownPlatform);
  }

  SECTION("generic") {
    REQUIRE(Source::convertPlatform("all") == Source::GenericPlatform);
  }

  SECTION("generic windows") {
    REQUIRE(Source::convertPlatform("windows") == Source::WindowsPlatform);
  }

  SECTION("windows 32-bit") {
    REQUIRE(Source::convertPlatform("win32") == Source::Win32Platform);
  }

  SECTION("windows 64-bit") {
    REQUIRE(Source::convertPlatform("win64") == Source::Win64Platform);
  }

  SECTION("generic os x") {
    REQUIRE(Source::convertPlatform("darwin") == Source::DarwinPlatform);
  }

  SECTION("os x 32-bit") {
    REQUIRE(Source::convertPlatform("darwin32") == Source::Darwin32Platform);
  }

  SECTION("os x 64-bit") {
    REQUIRE(Source::convertPlatform("darwin64") == Source::Darwin64Platform);
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

TEST_CASE("drow sources for unknown platforms") {
  Version ver("1");
  ver.addSource(make_shared<Source>(Source::UnknownPlatform, "a"));

  REQUIRE(ver.sources().size() == 0);
}

#ifdef __APPLE__
TEST_CASE("drop windows sources on os x", M) {
  Version ver("1");
  ver.addSource(make_shared<Source>(Source::WindowsPlatform, "a"));
  ver.addSource(make_shared<Source>(Source::Win32Platform, "a"));
  ver.addSource(make_shared<Source>(Source::Win64Platform, "a"));

  REQUIRE(ver.sources().size() == 0);
}

#ifdef __x86_64__
TEST_CASE("drop 32-bit sources on os x 64-bit", M) {
  Version ver("1");
  ver.addSource(make_shared<Source>(Source::Darwin32Platform, "a"));

  REQUIRE(ver.sources().size() == 0);
}

TEST_CASE("valid sources for os x 64-bit", M) {
  Version ver("1");
  ver.addSource(make_shared<Source>(Source::GenericPlatform, "a"));
  ver.addSource(make_shared<Source>(Source::DarwinPlatform, "a"));
  ver.addSource(make_shared<Source>(Source::Darwin64Platform, "a"));

  REQUIRE(ver.sources().size() == 3);
}
#else
TEST_CASE("drop 64-bit sources on os x 32-bit", M) {
  Version ver("1");
  ver.addSource(make_shared<Source>(Source::Darwin64Platform, "a"));

  REQUIRE(ver.sources().size() == 0);
}

TEST_CASE("valid sources for os x 32-bit", M) {
  Version ver("1");
  ver.addSource(make_shared<Source>(Source::GenericPlatform, "a"));
  ver.addSource(make_shared<Source>(Source::DarwinPlatform, "a"));
  ver.addSource(make_shared<Source>(Source::Darwin32Platform, "a"));

  REQUIRE(ver.sources().size() == 3);
}
#endif

#elif _WIN32
TEST_CASE("drop os x sources on windows", M) {
  Version ver("1");
  ver.addSource(make_shared<Source>(Source::DarwinPlatform, "a"));
  ver.addSource(make_shared<Source>(Source::Darwin32Platform, "a"));
  ver.addSource(make_shared<Source>(Source::Darwin64Platform, "a"));

  REQUIRE(ver.sources().size() == 0);
}

#ifdef _WIN64
TEST_CASE("drop 32-bit sources on windows 64-bit", M) {
  Version ver("1");
  ver.addSource(make_shared<Source>(Source::Win32Platform, "a"));

  REQUIRE(ver.sources().size() == 0);
}

TEST_CASE("valid sources for windows 64-bit", M) {
  Version ver("1");
  ver.addSource(make_shared<Source>(Source::GenericPlatform, "a"));
  ver.addSource(make_shared<Source>(Source::WindowsPlatform, "a"));
  ver.addSource(make_shared<Source>(Source::Win64Platform, "a"));

  REQUIRE(ver.sources().size() == 3);
}
#else
TEST_CASE("drop 64-bit sources on windows 32-bit", M) {
  Version ver("1");
  ver.addSource(make_shared<Source>(Source::Win64Platform, "a"));

  REQUIRE(ver.sources().size() == 0);
}

TEST_CASE("valid sources for windows 32-bit", M) {
  Version ver("1");
  ver.addSource(make_shared<Source>(Source::GenericPlatform, "a"));
  ver.addSource(make_shared<Source>(Source::WindowsPlatform, "a"));
  ver.addSource(make_shared<Source>(Source::Win32Platform, "a"));

  REQUIRE(ver.sources().size() == 3);
}
#endif
#endif
