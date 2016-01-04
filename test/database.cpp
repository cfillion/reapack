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

TEST_CASE("unicode database path", M) {
  Database *db = Database::load("a", DBPATH "Новая папка.xml");
  DBPTR(db);
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
  Version *ver = new Version("1", pack);
  Source *source = new Source(Source::GenericPlatform, {}, "google.com", ver);

  ver->addSource(source);
  pack->addVersion(ver);
  cat->addPackage(pack);

  CHECK(db.categories().size() == 0);

  db.addCategory(cat);

  REQUIRE(db.categories().size() == 1);
  REQUIRE(db.packages() == cat->packages());
}

TEST_CASE("add owned category", M) {
  Database db1("a");
  Database db2("b");

  Category *cat = new Category("name", &db1);

  try {
    db2.addCategory(cat);
    FAIL();
  }
  catch(const reapack_error &e) {
    delete cat;
    REQUIRE(string(e.what()) == "category belongs to another database");
  }
}

TEST_CASE("drop empty category", M) {
  Database db("a");
  db.addCategory(new Category("a", &db));

  REQUIRE(db.categories().empty());
}

TEST_CASE("add a package", M) {
  Database db("a");
  Category cat("a", &db);
  Package *pack = new Package(Package::ScriptType, "name", &cat);
  Version *ver = new Version("1", pack);
  ver->addSource(new Source(Source::GenericPlatform, {}, "google.com", ver));
  pack->addVersion(ver);

  CHECK(cat.packages().size() == 0);

  cat.addPackage(pack);

  REQUIRE(cat.packages().size() == 1);
  REQUIRE(pack->category() == &cat);
}

TEST_CASE("add owned package", M) {
  Category cat1("a");
  Package *pack = new Package(Package::ScriptType, "name", &cat1);

  try {
    Category cat2("b");
    cat2.addPackage(pack);
    FAIL();
  }
  catch(const reapack_error &e) {
    delete pack;
    REQUIRE(string(e.what()) == "package belongs to another category");
  }
}

TEST_CASE("drop empty package", M) {
  Category cat("a");
  cat.addPackage(new Package(Package::ScriptType, "name", &cat));

  REQUIRE(cat.packages().empty());
}

TEST_CASE("drop unknown package", M) {
  Category cat("a");
  cat.addPackage(new Package(Package::UnknownType, "name", &cat));

  REQUIRE(cat.packages().size() == 0);
}

TEST_CASE("empty category name", M) {
  try {
    Category cat{string(), nullptr};
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "empty category name");
  }
}

TEST_CASE("category full name", M) {
  Category cat1("Category Name");
  REQUIRE(cat1.fullName() == "Category Name");

  Database db("Database Name");
  Category cat2("Category Name", &db);
  REQUIRE(cat2.fullName() == "Database Name/Category Name");
}
