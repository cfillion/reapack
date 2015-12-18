#include <catch.hpp>

#include <database.hpp>
#include <errors.hpp>

#include <string>

#define DBPATH "test/db/"
#define DBPTR(ptr) unique_ptr<Database> dbptr(ptr)

using namespace std;

static const char *M = "[database]";

TEST_CASE("file not found", M) {
  try {
    Database *db = Database::load("a", DBPATH "404.xml");
    DBPTR(db);
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "failed to read database");
  }
}

TEST_CASE("broken xml", M) {
  try {
    Database *db = Database::load("a", DBPATH "broken.xml");
    DBPTR(db);
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "failed to read database");
  }
}

TEST_CASE("wrong root tag name", M) {
  try {
    Database *db = Database::load("a", DBPATH "wrong_root.xml");
    DBPTR(db);
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "invalid database");
  }
}

TEST_CASE("invalid version", M) {
  try {
    Database *db = Database::load("a", DBPATH "invalid_version.xml");
    DBPTR(db);
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "invalid version");
  }
}

TEST_CASE("future version", M) {
  try {
    Database *db = Database::load("a", DBPATH "future_version.xml");
    DBPTR(db);
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "unsupported version");
  }
}

TEST_CASE("empty database name", M) {
  try {
    Database cat{string()};
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "empty database name");
  }
}

TEST_CASE("add category", M) {
  Database db("a");
  Category *cat = new Category("a", &db);
  Package *pack = new Package(Package::ScriptType, "name", cat);
  Source *source = new Source(Source::GenericPlatform, string(), "google.com");
  Version *ver = new Version("1", pack);

  ver->addSource(source);
  pack->addVersion(ver);
  cat->addPackage(pack);

  CHECK(db.categories().size() == 0);

  db.addCategory(cat);

  REQUIRE(db.categories().size() == 1);
  REQUIRE(db.packages() == cat->packages());
}

TEST_CASE("drop empty category", M) {
  Database db("a");
  db.addCategory(new Category("a"));

  REQUIRE(db.categories().empty());
}

TEST_CASE("add a package", M) {
  Database db("a");
  Category cat1("a", &db);
  Package *pack = new Package(Package::ScriptType, "name", &cat1);
  Version *ver = new Version("1", pack);
  ver->addSource(new Source(Source::GenericPlatform, string(), "google.com"));
  pack->addVersion(ver);

  CHECK(pack->category() == &cat1);

  Category cat2("b");
  CHECK(cat2.packages().size() == 0);

  cat2.addPackage(pack);

  REQUIRE(cat2.packages().size() == 1);
  REQUIRE(pack->category() == &cat2);
}

TEST_CASE("drop empty package", M) {
  Category cat("a");
  cat.addPackage(new Package(Package::ScriptType, "name"));

  REQUIRE(cat.packages().empty());
}

TEST_CASE("drop unknown package", M) {
  Category cat("a");
  cat.addPackage(new Package(Package::UnknownType, "name"));

  REQUIRE(cat.packages().size() == 0);
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

TEST_CASE("category full name", M) {
  Category cat("Category Name");
  REQUIRE(cat.fullName() == "Category Name");

  Database db("Database Name");
  cat.setDatabase(&db);
  REQUIRE(cat.fullName() == "Database Name/Category Name");
}
