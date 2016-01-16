#include <catch.hpp>

#include <sqlite.hpp>

#include <errors.hpp>

using namespace std;

static const char *M = "[sqlite]";

TEST_CASE("open bad sqlite file path", M) {
  try {
    SQLite::Database db("/a\\");
    FAIL();
  }
  catch(const reapack_error &e) {}
}

TEST_CASE("execute invalid sql", M) {
  SQLite::Database db;

  try {
    db.query("WHERE");
    FAIL();
  }
  catch(const reapack_error &e) {}
}
