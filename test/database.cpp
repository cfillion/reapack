#include <catch.hpp>

#include <database.hpp>

#include <string>

#define DBPATH "test/db/"

using namespace std;

static const char *M = "[database]";

TEST_CASE("file not found", M) {
  const char *error = "";
  DatabasePtr db = Database::load(DBPATH "404.xml", &error);

  CHECK(db.get() == 0);
  REQUIRE(string(error) == "failed to read database");
}

TEST_CASE("broken xml", M) {
  const char *error = "";
  Database::load(DBPATH "broken.xml", &error);

  REQUIRE(string(error) == "failed to read database");
}

TEST_CASE("wrong root tag name", M) {
  const char *error = "";
  DatabasePtr db = Database::load(DBPATH "wrong_root.xml", &error);

  CHECK(db.get() == 0);
  REQUIRE(string(error) == "invalid database");
}

TEST_CASE("invalid version", M) {
  const char *error = "";
  DatabasePtr db = Database::load(DBPATH "invalid_version.xml", &error);

  CHECK(db.get() == 0);
  REQUIRE(string(error) == "invalid version");
}

TEST_CASE("future version", M) {
  const char *error = "";
  DatabasePtr db = Database::load(DBPATH "future_version.xml", &error);

  CHECK(db.get() == 0);
  REQUIRE(string(error) == "unsupported version");
}

TEST_CASE("add category to database", M) {
  Database db;
  CHECK(db.categories().size() == 0);

  db.addCategory(make_shared<Category>("a"));

  REQUIRE(db.categories().size() == 1);
}

TEST_CASE("add package to category", M) {
  PackagePtr pack = make_shared<Package>();

  Category cat("a");
  CHECK(cat.packages().size() == 0);
  CHECK_FALSE(pack->category());

  cat.addPackage(pack);

  REQUIRE(cat.packages().size() == 1);
  REQUIRE(pack->category() == &cat);
}

TEST_CASE("package type from string", M) {
  SECTION("null") {
    REQUIRE(Package::convertType(0) == Package::UnknownType);
  }

  SECTION("unknown") {
    REQUIRE(Package::convertType("yoyo") == Package::UnknownType);
  }

  SECTION("script") {
    REQUIRE(Package::convertType("script") == Package::ScriptType);
  }
}
