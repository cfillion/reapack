#include <catch.hpp>

#include <database.hpp>

#include <errors.hpp>

using namespace std;

static const char *M = "[database]";

TEST_CASE("open bad sqlite file path", M) {
  try {
    Database db("/a\\");
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "unable to open database file");
  }
}

TEST_CASE("execute invalid sql", M) {
  Database db;

  try {
    db.exec("WHERE");
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "near \"WHERE\": syntax error");
  }
}

TEST_CASE("prepare invalid sql", M) {
  Database db;

  try {
    db.prepare("WHERE");
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "near \"WHERE\": syntax error");
  }
}

TEST_CASE("get rows from prepared statement", M) {
  Database db;
  db.exec(
    "CREATE TABLE test (value TEXT NOT NULL);"
    "INSERT INTO test VALUES (\"hello\");"
    "INSERT INTO test VALUES (\"世界\");"
  );

  vector<string> values;

  Statement *stmt = db.prepare("SELECT value FROM test");

  SECTION("continue") {
    stmt->exec([&] {
      values.push_back(stmt->stringColumn(0));
      return true;
    });

    REQUIRE(values.size() == 2);
    REQUIRE(values[0] == "hello");
    REQUIRE(values[1] == "世界");
  }

  SECTION("abort") {
    stmt->exec([&] {
      values.push_back(stmt->stringColumn(0));
      return false;
    });

    REQUIRE(values.size() == 1);
    REQUIRE(values[0] == "hello");
  }
}

TEST_CASE("bing values and clear", M) {
  Database db;
  db.exec("CREATE TABLE test (value TEXT NOT NULL)");

  vector<string> values;

  Statement *stmt = db.prepare("INSERT INTO test VALUES (?)");
  stmt->bind(1, "hello");
  stmt->exec();

  try {
    stmt->exec();
    FAIL("bindings not cleared");
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "NOT NULL constraint failed: test.value");
  }
}

TEST_CASE("version", M) {
  Database db;
  REQUIRE(db.version() == 0);

  db.exec("PRAGMA user_version = 1");
  REQUIRE(db.version() == 1);
}
