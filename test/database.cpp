#include <catch.hpp>

#include <database.hpp>

#include <errors.hpp>

#include <sqlite3.h>

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

TEST_CASE("bind values and clear", M) {
  Database db;
  db.exec("CREATE TABLE test (value TEXT NOT NULL)");

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

TEST_CASE("foreign keys", M) {
  Database db;
  db.exec(
    "CREATE TABLE a (id INTEGER PRIMARY KEY);"

    "CREATE TABLE b ("
    "  two INTEGER NOT NULL,"
    "  FOREIGN KEY(two) REFERENCES a(id)"
    ");"

    "INSERT INTO a VALUES(NULL);"
    "INSERT INTO b VALUES(1);"
  );

  try {
    db.exec("DELETE FROM a");
    FAIL("foreign keys checks are disabled");
  }
  catch(const reapack_error &) {}
}

TEST_CASE("last insert id", M) {
  Database db;
  db.exec("CREATE TABLE a(text TEXT)");

  Statement *insert = db.prepare("INSERT INTO a VALUES(NULL)");

  REQUIRE(db.lastInsertId() == 0);

  insert->exec();
  REQUIRE(db.lastInsertId() == 1);

  insert->exec();
  REQUIRE(db.lastInsertId() == 2);
}

TEST_CASE("bind temporary strings", M) {
  Database db;
  db.exec("CREATE TABLE a(text TEXT NOT NULL)");

  Statement *insert = db.prepare("INSERT INTO a VALUES(?)");

  string str("hello");
  insert->bind(1, str);
  str = "world";

  insert->exec();

  string got;
  Statement *select = db.prepare("SELECT text FROM a LIMIT 1");
  select->exec([&] {
    got = select->stringColumn(0);
    return false;
  });

  REQUIRE(got == "hello");
}

TEST_CASE("sqlite error code", M) {
  Database db;
  db.exec("CREATE TABLE a(b INTEGER UNIQUE); INSERT INTO a VALUES(1)");
  REQUIRE(db.errorCode() == SQLITE_OK);

  try {
    db.exec("INSERT INTO a VALUES(1)");
  }
  catch(const reapack_error &) {}

  REQUIRE(db.errorCode() == SQLITE_CONSTRAINT);
}

TEST_CASE("invalid string column", M) {
  Database db;
  db.exec("CREATE TABLE a(text TEXT NOT NULL)");

  Statement *insert = db.prepare("INSERT INTO a VALUES(?)");
  insert->bind(1, "hello");
  insert->exec();

  Statement *select = db.prepare("SELECT text FROM a LIMIT 1");
  select->exec([&] {
    REQUIRE(select->stringColumn(4242).empty()); // don't crash!
    return false;
  });
}
