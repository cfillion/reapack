#include <catch.hpp>

#include <index.hpp>
#include <source.hpp>
#include <version.hpp>

#include <errors.hpp>

using namespace std;

static const char *M = "[source]";

TEST_CASE("source platform from string", M) {
  SECTION("unknown")
    REQUIRE(Source::getPlatform("hello") == Source::UnknownPlatform);

  SECTION("generic")
    REQUIRE(Source::getPlatform("all") == Source::GenericPlatform);

  SECTION("generic windows")
    REQUIRE(Source::getPlatform("windows") == Source::WindowsPlatform);

  SECTION("windows 32-bit")
    REQUIRE(Source::getPlatform("win32") == Source::Win32Platform);

  SECTION("windows 64-bit")
    REQUIRE(Source::getPlatform("win64") == Source::Win64Platform);

  SECTION("generic os x")
    REQUIRE(Source::getPlatform("darwin") == Source::DarwinPlatform);

  SECTION("os x 32-bit")
    REQUIRE(Source::getPlatform("darwin32") == Source::Darwin32Platform);

  SECTION("os x 64-bit")
    REQUIRE(Source::getPlatform("darwin64") == Source::Darwin64Platform);
}

TEST_CASE("source platform", M) {
  Source src({}, "url");
  REQUIRE(src.platform() == Source::GenericPlatform);

  src.setPlatform(Source::UnknownPlatform);
  REQUIRE(src.platform() == Source::UnknownPlatform);

  src.setPlatform(Source::WindowsPlatform);
  REQUIRE(src.platform() == Source::WindowsPlatform);
}

TEST_CASE("empty file name and no package", M) {
  const Source source({}, "url");

  try {
    (void)source.file();
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "empty file name and no package");
  }
}

TEST_CASE("main source", M) {
  SECTION("with file name") {
    const Source source("filename", "url");
    REQUIRE_FALSE(source.isMain());
  }

  SECTION("without file name") {
    const Source source({}, "url");
    REQUIRE(source.isMain());
  }
}

TEST_CASE("empty source url", M) {
  try {
    const Source source("filename", {});
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "empty source url");
  }
}

TEST_CASE("full name without version", M) {
  SECTION("with file name") {
    const Source source("path/to/file", "b");
    REQUIRE(source.fullName() == "file");
  }

  SECTION("without file name") {
    try {
      const Source source({}, "b");
      (void)source.fullName();
      FAIL();
    }
    catch(const reapack_error &e) {
      REQUIRE(string(e.what()) == "empty file name and no package");
    }
  }
}

TEST_CASE("full name with version", M) {
  SECTION("with file name") {
    Version ver("1.0");
    const Source source("path/to/file", "b", &ver);

    REQUIRE(source.fullName() == "v1.0 (file)");
  }

  SECTION("without file name") {
    Version ver("1.0");
    const Source source({}, "b", &ver);

    REQUIRE(source.fullName() == ver.fullName());
  }
}

TEST_CASE("source target path", M) {
  Index ri("Index Name");
  Category cat("Category Name", &ri);
  Package pack(Package::ScriptType, "package name", &cat);
  Version ver("1.0", &pack);

  const Source source("file.name", "url", &ver);

  Path expected;
  expected.append("Scripts");
  expected.append("Index Name");
  expected.append("Category Name");
  expected.append("file.name");

  REQUIRE(source.targetPath() == expected);
}

TEST_CASE("source target path with parent directory traversal", M) {
  Index ri("Index Name");
  Category cat("Category Name", &ri);
  Package pack(Package::ScriptType, "package name", &cat);
  Version ver("1.0", &pack);

  const Source source("../../../file.name", "url", &ver);

  Path expected;
  expected.append("Scripts");
  expected.append("Index Name");
  // expected.append("Category Name"); // only the category can be bypassed!
  expected.append("file.name");

  REQUIRE(source.targetPath() == expected);
}

TEST_CASE("source target path without package", M) {
  try {
    const Source source("a", "b");
    (void)source.targetPath();
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "no package associated with the source");
  }
}
