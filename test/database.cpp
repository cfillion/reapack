#include <catch.hpp>

#include <database.hpp>
#include <errors.hpp>

#include <string>

#define DBPATH "test/db/"

using namespace std;

static const char *M = "[database]";

TEST_CASE("file not found", M) {
  try {
    Database::load(DBPATH "404.xml");
    FAIL();
  }
  catch(const database_error &e) {
    REQUIRE(string(e.what()) == "failed to read database");
  }
}

TEST_CASE("broken xml", M) {
  try {
    Database::load(DBPATH "broken.xml");
    FAIL();
  }
  catch(const database_error &e) {
    REQUIRE(string(e.what()) == "failed to read database");
  }
}

TEST_CASE("wrong root tag name", M) {
  try {
    Database::load(DBPATH "wrong_root.xml");
    FAIL();
  }
  catch(const database_error &e) {
    REQUIRE(string(e.what()) == "invalid database");
  }
}

TEST_CASE("invalid version", M) {
  try {
    Database::load(DBPATH "invalid_version.xml");
    FAIL();
  }
  catch(const database_error &e) {
    REQUIRE(string(e.what()) == "invalid version");
  }
}

TEST_CASE("future version", M) {
  try {
    Database::load(DBPATH "future_version.xml");
    FAIL();
  }
  catch(const database_error &e) {
    REQUIRE(string(e.what()) == "unsupported version");
  }
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
