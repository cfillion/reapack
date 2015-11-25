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
