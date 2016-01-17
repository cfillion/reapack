#include <catch.hpp>

#include <errors.hpp>
#include <index.hpp>

#include <string>

#define RIPATH "test/indexes/"
#define RIPTR(ptr) unique_ptr<RemoteIndex> riptr(ptr)

using namespace std;

static const char *M = "[index]";

TEST_CASE("file not found", M) {
  try {
    RemoteIndex *ri = RemoteIndex::load("a", RIPATH "404.xml");
    RIPTR(ri);
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "failed to read index");
  }
}

TEST_CASE("broken xml", M) {
  try {
    RemoteIndex *ri = RemoteIndex::load("a", RIPATH "broken.xml");
    RIPTR(ri);
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "failed to read index");
  }
}

TEST_CASE("wrong root tag name", M) {
  try {
    RemoteIndex *ri = RemoteIndex::load("a", RIPATH "wrong_root.xml");
    RIPTR(ri);
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "invalid index");
  }
}

TEST_CASE("invalid version", M) {
  try {
    RemoteIndex *ri = RemoteIndex::load("a", RIPATH "invalid_version.xml");
    RIPTR(ri);
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "invalid version");
  }
}

TEST_CASE("future version", M) {
  try {
    RemoteIndex *ri = RemoteIndex::load("a", RIPATH "future_version.xml");
    RIPTR(ri);
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "unsupported version");
  }
}

TEST_CASE("unicode index path", M) {
  RemoteIndex *ri = RemoteIndex::load("a", RIPATH "Новая папка.xml");
  RIPTR(ri);
}

TEST_CASE("empty index name", M) {
  try {
    RemoteIndex cat{string()};
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "empty index name");
  }
}

TEST_CASE("add category", M) {
  RemoteIndex ri("a");
  Category *cat = new Category("a", &ri);
  Package *pack = new Package(Package::ScriptType, "name", cat);
  Version *ver = new Version("1", pack);
  Source *source = new Source(Source::GenericPlatform, {}, "google.com", ver);

  ver->addSource(source);
  pack->addVersion(ver);
  cat->addPackage(pack);

  CHECK(ri.categories().size() == 0);

  ri.addCategory(cat);

  REQUIRE(ri.categories().size() == 1);
  REQUIRE(ri.packages() == cat->packages());
}

TEST_CASE("add owned category", M) {
  RemoteIndex ri1("a");
  RemoteIndex ri2("b");

  Category *cat = new Category("name", &ri1);

  try {
    ri2.addCategory(cat);
    FAIL();
  }
  catch(const reapack_error &e) {
    delete cat;
    REQUIRE(string(e.what()) == "category belongs to another index");
  }
}

TEST_CASE("drop empty category", M) {
  RemoteIndex ri("a");
  ri.addCategory(new Category("a", &ri));

  REQUIRE(ri.categories().empty());
}

TEST_CASE("add a package", M) {
  RemoteIndex ri("a");
  Category cat("a", &ri);
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

  RemoteIndex ri("Remote Name");
  Category cat2("Category Name", &ri);
  REQUIRE(cat2.fullName() == "Remote Name/Category Name");
}
