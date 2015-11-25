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
