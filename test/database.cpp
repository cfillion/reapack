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

TEST_CASE("database version", M) {
  Database db;
  REQUIRE(db.version().major == 0);
  REQUIRE(db.version().minor == 0);
  REQUIRE_FALSE(db.version());
  REQUIRE_FALSE(db.version() < (Database::Version{0, 0}));
  REQUIRE(db.version() < (Database::Version{0, 1}));
  REQUIRE(db.version() < (Database::Version{1, 0}));

  db.setVersion({0, 1});
  REQUIRE(db.version().major == 0);
  REQUIRE(db.version().minor == 1);
  REQUIRE(db.version());

  db.setVersion({1, 2});
  REQUIRE(db.version() < (Database::Version{2, 3}));
  REQUIRE_FALSE(db.version() < (Database::Version{1, 1}));
  REQUIRE_FALSE(db.version() < (Database::Version{0, 3}));

  db.setVersion({32767, 32767});
  REQUIRE(db.version().major == 32767);
  REQUIRE(db.version().minor == 32767);
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

TEST_CASE("get integers from sqlite", M) {
  Database db;
  db.exec("CREATE TABLE a(test INTEGER NOT NULL)");

  Statement *insert = db.prepare("INSERT INTO a VALUES(?)");

  insert->bind(1, 2147483647);
  insert->exec();
  insert->bind(1, 4294967295);
  insert->exec();

  vector<sqlite3_int64> signedVals;
  Statement *select = db.prepare("SELECT test FROM a");
  select->exec([&] {
    signedVals.push_back(select->intColumn(0));
    return true;
  });

  CHECK(signedVals.size() == 2);
  REQUIRE(signedVals[0] == 2147483647);
  REQUIRE(signedVals[1] == 4294967295);
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

TEST_CASE("database transaction locking", M) {
  Database db;
  db.begin();

  try {
    db.begin();
    FAIL("created a transaction within a transaction");
  }
  catch(const reapack_error &) {}

  db.commit();
  db.begin();
}

TEST_CASE("save points", M) {
  Database db;
  db.exec("CREATE TABLE test (value INTEGER NOT NULL);");

  Statement *insert = db.prepare("INSERT INTO test VALUES (1);");
  Statement *select = db.prepare("SELECT COUNT(*) FROM test");

  auto count = [select] {
    int count = -255;
    select->exec([&] { count = select->intColumn(0); return false; });
    return count;
  };

  db.savepoint();

  insert->exec();
  REQUIRE(count() == 1);

  SECTION("rollback to savepoint") {
    db.restore();
    REQUIRE(count() == 0);
    try { db.restore(); FAIL("rolled back unexistant savepoint"); }
    catch(const reapack_error &) {}
  }

  SECTION("release savepoint") {
    db.release();
    REQUIRE(count() == 1);
    try { db.release(); FAIL("released unexistant savepoint"); }
    catch(const reapack_error &) {}
  }
}
