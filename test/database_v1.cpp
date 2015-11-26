#include <catch.hpp>

#include <database.hpp>

#include <string>

#define DBPATH "test/db/v1/"

using namespace std;

static const char *M = "[database_v1]";

TEST_CASE("unnamed category", M) {
  const char *error = "";
  DatabasePtr db = Database::load(DBPATH "unnamed_category.xml", &error);

  CHECK(db.get() == 0);
  REQUIRE(string(error) == "missing category name");
}

TEST_CASE("empty category", M) {
  const char *error = 0;
  DatabasePtr db = Database::load(DBPATH "empty_category.xml", &error);

  REQUIRE(error == 0);
  REQUIRE(db->categories().size() == 1);
  REQUIRE(db->category(0)->name() == "Hello World");
  REQUIRE(db->category(0)->packages().empty());
}

TEST_CASE("invalid category tag", M) {
  const char *error = "";
  DatabasePtr db = Database::load(DBPATH "wrong_category_tag.xml", &error);

  CHECK(db.get() == 0);
  REQUIRE(string(error) == "not a category");
}

TEST_CASE("unnamed package", M) {
  const char *error = "";
  DatabasePtr db = Database::load(DBPATH "unnamed_package.xml", &error);

  CHECK(db.get() == 0);
  REQUIRE(string(error) == "missing package name");
}

TEST_CASE("invalid package type", M) {
  const char *error = "";
  DatabasePtr db = Database::load(DBPATH "invalid_type.xml", &error);

  CHECK(db.get() == 0);
  REQUIRE(string(error) == "unsupported package type");
}

TEST_CASE("anonymous package", M) {
  const char *error = "";
  DatabasePtr db = Database::load(DBPATH "anonymous_package.xml", &error);

  CHECK(db.get() == 0);
  REQUIRE(string(error) == "package is anonymous");
}
