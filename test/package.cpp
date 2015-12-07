#include <catch.hpp>

#include "helper/io.hpp"

#include <database.hpp>
#include <errors.hpp>
#include <package.hpp>

#include <string>

using namespace std;

static const char *M = "[package]";

TEST_CASE("empty package name", M) {
  try {
    Package pack(Package::ScriptType, string());
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "empty package name");
  }
}

TEST_CASE("package versions are sorted", M) {
  Package pack(Package::ScriptType, "a");
  CHECK(pack.versions().size() == 0);

  Source *sourceA = new Source(Source::GenericPlatform, "google.com");
  Source *sourceB = new Source(Source::GenericPlatform, "google.com");

  Version *final = new Version("1");
  final->addSource(sourceA);

  Version *alpha = new Version("0.1");
  alpha->addSource(sourceB);

  pack.addVersion(final);
  REQUIRE(final->package() == &pack);
  CHECK(pack.versions().size() == 1);

  pack.addVersion(alpha);
  CHECK(pack.versions().size() == 2);

  REQUIRE(pack.version(0) == alpha);
  REQUIRE(pack.version(1) == final);
  REQUIRE(pack.lastVersion() == final);
}

TEST_CASE("drop empty version", M) {
  Package pack(Package::ScriptType, "a");
  pack.addVersion(new Version("1"));

  REQUIRE(pack.versions().empty());
  REQUIRE(pack.lastVersion() == 0);
}

TEST_CASE("unknown target path", M) {
  Database db;
  Category cat("name");
  cat.setDatabase(&db);

  Package pack(Package::UnknownType, "a");
  pack.setCategory(&cat);

  try {
    pack.targetPath();
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "unsupported package type");
  }
}

TEST_CASE("script target path", M) {
  Database db;
  db.setName("Database Name");

  Category cat("Category Name");
  cat.setDatabase(&db);

  Package pack(Package::ScriptType, "file.name");
  pack.setCategory(&cat);

  const Path path = pack.targetPath();

  Path expected;
  expected.append("Scripts");
  expected.append("Database Name");
  expected.append("Category Name");
  expected.append("file.name");

  REQUIRE(path == expected);
}

TEST_CASE("script target path without category", M) {
  Package pack(Package::ScriptType, "file.name");

  try {
    pack.targetPath();
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "category or database is unset");
  }
}

TEST_CASE("full name", M) {
  Database db;
  db.setName("Database Name");

  Category cat("Category Name");

  Package pack(Package::ScriptType, "file.name");
  REQUIRE(pack.fullName() == "file.name");

  pack.setCategory(&cat);
  REQUIRE(pack.fullName() == "Category Name/file.name");

  cat.setDatabase(&db);
  REQUIRE(pack.fullName() == "Database Name/Category Name/file.name");
}
