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
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "failed to read database");
  }
}

TEST_CASE("broken xml", M) {
  try {
    Database::load(DBPATH "broken.xml");
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "failed to read database");
  }
}

TEST_CASE("wrong root tag name", M) {
  try {
    Database::load(DBPATH "wrong_root.xml");
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "invalid database");
  }
}

TEST_CASE("invalid version", M) {
  try {
    Database::load(DBPATH "invalid_version.xml");
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "invalid version");
  }
}

TEST_CASE("future version", M) {
  try {
    Database::load(DBPATH "future_version.xml");
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "unsupported version");
  }
}
TEST_CASE("add category", M) {
  VersionPtr ver = make_shared<Version>("1");
  ver->addSource(make_shared<Source>(Source::GenericPlatform, "google.com"));

  PackagePtr pack = make_shared<Package>(Package::ScriptType, "name");
  pack->addVersion(ver);

  CategoryPtr cat = make_shared<Category>("a");
  cat->addPackage(pack);

  Database db;
  CHECK(db.categories().size() == 0);

  db.addCategory(cat);

  REQUIRE(db.categories().size() == 1);
  REQUIRE(db.packages() == cat->packages());
}

TEST_CASE("drop empty category", M) {
  Database db;
  db.addCategory(make_shared<Category>("a"));

  REQUIRE(db.categories().empty());
}

TEST_CASE("add a package", M) {
  VersionPtr ver = make_shared<Version>("1");
  ver->addSource(make_shared<Source>(Source::GenericPlatform, "google.com"));

  PackagePtr pack = make_shared<Package>(Package::ScriptType, "name");
  pack->addVersion(ver);

  Category cat("a");
  CHECK(cat.packages().size() == 0);
  CHECK_FALSE(pack->category());

  cat.addPackage(pack);

  REQUIRE(cat.packages().size() == 1);
  REQUIRE(pack->category() == &cat);
}

TEST_CASE("drop empty package", M) {
  Category cat("a");
  cat.addPackage(make_shared<Package>(Package::ScriptType, "name"));

  REQUIRE(cat.packages().empty());
}

TEST_CASE("drop unknown package", M) {
  Category cat("a");
  cat.addPackage(make_shared<Package>(Package::UnknownType, "name"));

  REQUIRE(cat.packages().size() == 0);
}

TEST_CASE("package type from string", M) {
  SECTION("unknown") {
    REQUIRE(Package::convertType("yoyo") == Package::UnknownType);
  }

  SECTION("script") {
    REQUIRE(Package::convertType("script") == Package::ScriptType);
  }
}

TEST_CASE("empty category name", M) {
  try {
    Category cat{string()};
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "empty category name");
  }
}
