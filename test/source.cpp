#include <catch.hpp>

#include <database.hpp>
#include <source.hpp>
#include <version.hpp>

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

TEST_CASE("empty file name and no package", M) {
  Source source(Source::UnknownPlatform, string(), "b");

  try {
    (void)source.file();
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "empty file name and no package");
  }
}

TEST_CASE("empty source url", M) {
  try {
    Source source(Source::UnknownPlatform, "a", string());
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "empty source url");
  }
}

TEST_CASE("full name without version", M) {
  SECTION("source name") {
    Source source(Source::UnknownPlatform, "a", "b");
    REQUIRE(source.fullName() == "a");
  }

  SECTION("package name") {
    try {
      Source source(Source::UnknownPlatform, string(), "b");
      (void)source.fullName();
      FAIL();
    }
    catch(const reapack_error &e) {
      REQUIRE(string(e.what()) == "empty file name and no package");
    }
  }
}

TEST_CASE("full name with version", M) {
  SECTION("with source name") {
    Version ver("1.0");
    Source source(Source::UnknownPlatform, "a", "b");
    source.setVersion(&ver);

    REQUIRE(source.fullName() == "v1.0 (a)");
  }

  SECTION("with source name") {
    Version ver("1.0");
    Source source(Source::UnknownPlatform, string(), "b");
    source.setVersion(&ver);

    REQUIRE(source.fullName() == ver.fullName());
  }
}

TEST_CASE("source target path", M) {
  Database db;
  db.setName("Database Name");

  Category cat("Category Name");
  cat.setDatabase(&db);

  Package pack(Package::ScriptType, "package name");
  pack.setCategory(&cat);

  Version ver("1.0");
  ver.setPackage(&pack);

  Source source(Source::GenericPlatform, "file.name", "https://google.com");
  source.setVersion(&ver);

  Path expected;
  expected.append("Scripts");
  expected.append("Database Name");
  expected.append("Category Name");
  expected.append("file.name");

  REQUIRE(source.targetPath() == expected);
}

TEST_CASE("source target path without package", M) {
  try {
    Source source(Source::GenericPlatform, "a", "b");
    (void)source.targetPath();
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "no package associated with the source");
  }
}
