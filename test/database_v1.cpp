#include <catch.hpp>

#include <database.hpp>
#include <errors.hpp>

#include <string>

#define DBPATH "test/db/v1/"

using namespace std;

static const char *M = "[database_v1]";

TEST_CASE("unnamed category", M) {
  try {
    Database::load(DBPATH "unnamed_category.xml");
    FAIL();
  }
  catch(const database_error &e) {
    REQUIRE(string(e.what()) == "missing category name");
  }
}

TEST_CASE("empty category", M) {
  DatabasePtr db = Database::load(DBPATH "empty_category.xml");

  REQUIRE(db->categories().size() == 1);
  REQUIRE(db->category(0)->name() == "Hello World");
  REQUIRE(db->category(0)->packages().empty());
}

TEST_CASE("invalid category tag", M) {
  try {
    Database::load(DBPATH "wrong_category_tag.xml");
    FAIL();
  }
  catch(const database_error &e) {
    REQUIRE(string(e.what()) == "not a category");
  }
}

TEST_CASE("unnamed package", M) {
  try {
    Database::load(DBPATH "unnamed_package.xml");
    FAIL();
  }
  catch(const database_error &e) {
    REQUIRE(string(e.what()) == "missing package name");
  }
}

TEST_CASE("invalid package type", M) {
  try {
    Database::load(DBPATH "invalid_type.xml");
    FAIL();
  }
  catch(const database_error &e) {
    REQUIRE(string(e.what()) == "unsupported package type");
  }
}

TEST_CASE("anonymous package", M) {
  try {
    Database::load(DBPATH "anonymous_package.xml");
    FAIL();
  }
  catch(const database_error &e) {
    REQUIRE(string(e.what()) == "package is anonymous");
  }
}
