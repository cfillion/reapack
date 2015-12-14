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
    Database *db = Database::load(DBPATH "404.xml");
    DBPTR(db);
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "failed to read database");
  }
}

TEST_CASE("broken xml", M) {
  try {
    Database *db = Database::load(DBPATH "broken.xml");
    DBPTR(db);
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "failed to read database");
  }
}

TEST_CASE("wrong root tag name", M) {
  try {
    Database *db = Database::load(DBPATH "wrong_root.xml");
    DBPTR(db);
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "invalid database");
  }
}

TEST_CASE("invalid version", M) {
  try {
    Database *db = Database::load(DBPATH "invalid_version.xml");
    DBPTR(db);
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "invalid version");
  }
}

TEST_CASE("future version", M) {
  try {
    Database *db = Database::load(DBPATH "future_version.xml");
    DBPTR(db);
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "unsupported version");
  }
}
TEST_CASE("add category", M) {
  Version *ver = new Version("1");
  ver->addSource(new Source(Source::GenericPlatform, string(), "google.com"));

  Package *pack = new Package(Package::ScriptType, "name");
  pack->addVersion(ver);

  Category *cat = new Category("a");
  cat->addPackage(pack);

  Database db;
  CHECK(db.categories().size() == 0);

  db.addCategory(cat);

  REQUIRE(db.categories().size() == 1);
  REQUIRE(db.packages() == cat->packages());
}

TEST_CASE("drop empty category", M) {
  Database db;
  db.addCategory(new Category("a"));

  REQUIRE(db.categories().empty());
}

TEST_CASE("add a package", M) {
  Version *ver = new Version("1");
  ver->addSource(new Source(Source::GenericPlatform, string(), "google.com"));

  Package *pack = new Package(Package::ScriptType, "name");
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

  Database db;
  db.setName("Database Name");
  cat.setDatabase(&db);
  REQUIRE(cat.fullName() == "Database Name/Category Name");
}
