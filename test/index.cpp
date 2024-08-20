#include "helper.hpp"

#include <errors.hpp>
#include <index.hpp>

static const char *M = "[index]";
static const Path RIPATH("test/indexes");

TEST_CASE("index file not found", M) {
  UseRootPath root(RIPATH);

  try {
    IndexPtr ri = Index::load("404");
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(std::string{e.what()} == "No such file or directory");
  }
}

TEST_CASE("load index from raw data", M) {
  SECTION("valid") {
    Index::load({}, "<index version=\"1\"/>\n");
  }

  SECTION("broken") {
    try {
      Index::load({}, "<index>\n");
      FAIL();
    }
    catch(const reapack_error &) {}
  }
}

TEST_CASE("no root node", M) {
  try {
    Index::load({}, R"(<?xml verison="1.0 encoding="utf-8"?>)");
    FAIL();
  }
  catch(const reapack_error &) {}
}

TEST_CASE("broken index", M) {
  UseRootPath root(RIPATH);

  try {
    IndexPtr ri = Index::load("broken");
    FAIL();
  }
  catch(const reapack_error &) {}
}

TEST_CASE("wrong root tag name", M) {
  UseRootPath root(RIPATH);

  try {
    IndexPtr ri = Index::load("wrong_root");
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(std::string{e.what()} == "invalid index");
  }
}

TEST_CASE("invalid version", M) {
  UseRootPath root(RIPATH);

  try {
    IndexPtr ri = Index::load("invalid_version");
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(std::string{e.what()} == "index version not found");
  }
}

TEST_CASE("future version", M) {
  UseRootPath root(RIPATH);

  try {
    IndexPtr ri = Index::load("future_version");
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(std::string{e.what()} == "index version is unsupported");
  }
}

TEST_CASE("add a category", M) {
  Index ri("a");
  Category *cat = new Category("a", &ri);
  Package *pack = new Package(Package::ScriptType, "name", cat, "remote");
  Version *ver = new Version("1", pack);
  Source *source = new Source({}, "google.com", ver);

  ver->addSource(source);
  pack->addVersion(ver);
  cat->addPackage(pack);

  CHECK(ri.categories().size() == 0);
  CHECK(ri.category("a") == nullptr);

  REQUIRE(ri.addCategory(cat));

  REQUIRE(ri.categories().size() == 1);
  REQUIRE(ri.category("a") == cat);
  REQUIRE(ri.packages() == cat->packages());
}

TEST_CASE("add owned category", M) {
  Index ri1("a");
  Index ri2("b");

  Category *cat = new Category("name", &ri1);

  try {
    ri2.addCategory(cat);
    FAIL();
  }
  catch(const reapack_error &e) {
    delete cat;
    REQUIRE(std::string{e.what()} == "category belongs to another index");
  }
}

TEST_CASE("drop empty category", M) {
  Index ri("a");
  const Category cat("a", &ri);
  REQUIRE_FALSE(ri.addCategory(&cat));

  REQUIRE(ri.categories().empty());
}

TEST_CASE("add a package", M) {
  Index ri("a");
  Category cat("a", &ri);
  Package *pack = new Package(Package::ScriptType, "name", &cat, "remote");
  Version *ver = new Version("1", pack);
  ver->addSource(new Source({}, "google.com", ver));
  pack->addVersion(ver);

  CHECK(cat.packages().size() == 0);
  CHECK(cat.package("name") == nullptr);

  REQUIRE(cat.addPackage(pack));

  REQUIRE(cat.packages().size() == 1);
  REQUIRE(cat.package("name") == pack);
  REQUIRE(pack->category() == &cat);
}

TEST_CASE("add owned package", M) {
  Category cat1("a", nullptr);
  Package *pack = new Package(Package::ScriptType, "name", &cat1, "remote");

  try {
    Category cat2("b", nullptr);
    cat2.addPackage(pack);
    FAIL();
  }
  catch(const reapack_error &e) {
    delete pack;
    REQUIRE(std::string{e.what()} == "package belongs to another category");
  }
}

TEST_CASE("drop empty package", M) {
  Category cat("a", nullptr);
  const Package pkg(Package::ScriptType, "name", &cat, "remote");
  REQUIRE_FALSE(cat.addPackage(&pkg));
  REQUIRE(cat.packages().empty());
}

TEST_CASE("drop unknown package", M) {
  Category cat("a", nullptr);
  const Package pkg(Package::UnknownType, "name", &cat, "remote");
  REQUIRE_FALSE(cat.addPackage(&pkg));
  REQUIRE(cat.packages().size() == 0);
}

TEST_CASE("empty category name", M) {
  try {
    Category cat{{}, nullptr};
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(std::string{e.what()} == "empty category name");
  }
}

TEST_CASE("category full name", M) {
  Index ri("Remote Name");
  Category cat2("Category Name", &ri);
  REQUIRE(cat2.fullName() == "Remote Name/Category Name");
}

TEST_CASE("set index name", M) {
  SECTION("set") {
    Index ri({});
    ri.setName("Hello/World!");
    REQUIRE(ri.name() == "Hello/World!");
  }

  SECTION("override") {
    Index ri("hello");
    try {
      ri.setName("world");
      FAIL();
    }
    catch(const reapack_error &e) {
      REQUIRE(std::string{e.what()} == "index name is already set");
    }
    REQUIRE(ri.name() == "hello");
  }
}

TEST_CASE("find package", M) {
  Index ri("index name");
  Category *cat = new Category("cat", &ri);
  Package *pack = new Package(Package::ScriptType, "pkg", cat, "remote");
  Version *ver = new Version("1", pack);
  Source *source = new Source({}, "google.com", ver);

  ver->addSource(source);
  pack->addVersion(ver);
  cat->addPackage(pack);
  ri.addCategory(cat);

  REQUIRE(ri.find("a", "b") == nullptr);
  REQUIRE(ri.find("cat", "b") == nullptr);
  REQUIRE(ri.find("cat", "pkg") == pack);
}
