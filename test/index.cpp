#include "helper.hpp"

#include <errors.hpp>
#include <index.hpp>

#include <string>

using namespace std;

static const char *M = "[index]";
static const Path RIPATH(L("test/indexes"));

TEST_CASE("index file not found", M) {
  UseRootPath root(RIPATH);

  try {
    IndexPtr ri = Index::load(L("404"));
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(e.what() == L("No such file or directory"));
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
    catch(const reapack_error &e) {
      REQUIRE(e.what() == L("Error reading end tag."));
    }
  }
}

TEST_CASE("broken index", M) {
  UseRootPath root(RIPATH);

  try {
    IndexPtr ri = Index::load(L("broken"));
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(e.what() == L("Error reading end tag."));
  }
}

TEST_CASE("wrong root tag name", M) {
  UseRootPath root(RIPATH);

  try {
    IndexPtr ri = Index::load(L("wrong_root"));
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(e.what() == L("invalid index"));
  }
}

TEST_CASE("invalid version", M) {
  UseRootPath root(RIPATH);

  try {
    IndexPtr ri = Index::load(L("invalid_version"));
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(e.what() == L("index version not found"));
  }
}

TEST_CASE("future version", M) {
  UseRootPath root(RIPATH);

  try {
    IndexPtr ri = Index::load(L("future_version"));
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(e.what() == L("index version is unsupported"));
  }
}

TEST_CASE("add a category", M) {
  Index ri(L("a"));
  Category *cat = new Category(L("a"), &ri);
  Package *pack = new Package(Package::ScriptType, L("name"), cat);
  Version *ver = new Version(L("1"), pack);
  Source *source = new Source({}, L("google.com"), ver);

  ver->addSource(source);
  pack->addVersion(ver);
  cat->addPackage(pack);

  CHECK(ri.categories().size() == 0);
  CHECK(ri.category(L("a")) == nullptr);

  REQUIRE(ri.addCategory(cat));

  REQUIRE(ri.categories().size() == 1);
  REQUIRE(ri.category(L("a")) == cat);
  REQUIRE(ri.packages() == cat->packages());
}

TEST_CASE("add owned category", M) {
  Index ri1(L("a"));
  Index ri2(L("b"));

  Category *cat = new Category(L("name"), &ri1);

  try {
    ri2.addCategory(cat);
    FAIL();
  }
  catch(const reapack_error &e) {
    delete cat;
    REQUIRE(e.what() == L("category belongs to another index"));
  }
}

TEST_CASE("drop empty category", M) {
  Index ri(L("a"));
  const Category cat(L("a"), &ri);
  REQUIRE_FALSE(ri.addCategory(&cat));

  REQUIRE(ri.categories().empty());
}

TEST_CASE("add a package", M) {
  Index ri(L("a"));
  Category cat(L("a"), &ri);
  Package *pack = new Package(Package::ScriptType, L("name"), &cat);
  Version *ver = new Version(L("1"), pack);
  ver->addSource(new Source({}, L("google.com"), ver));
  pack->addVersion(ver);

  CHECK(cat.packages().size() == 0);
  CHECK(cat.package(L("name")) == nullptr);

  REQUIRE(cat.addPackage(pack));

  REQUIRE(cat.packages().size() == 1);
  REQUIRE(cat.package(L("name")) == pack);
  REQUIRE(pack->category() == &cat);
}

TEST_CASE("add owned package", M) {
  Category cat1(L("a"), nullptr);
  Package *pack = new Package(Package::ScriptType, L("name"), &cat1);

  try {
    Category cat2(L("b"), nullptr);
    cat2.addPackage(pack);
    FAIL();
  }
  catch(const reapack_error &e) {
    delete pack;
    REQUIRE(e.what() == L("package belongs to another category"));
  }
}

TEST_CASE("drop empty package", M) {
  Category cat(L("a"), nullptr);
  const Package pkg(Package::ScriptType, L("name"), &cat);
  REQUIRE_FALSE(cat.addPackage(&pkg));
  REQUIRE(cat.packages().empty());
}

TEST_CASE("drop unknown package", M) {
  Category cat(L("a"), nullptr);
  const Package pkg(Package::UnknownType, L("name"), &cat);
  REQUIRE_FALSE(cat.addPackage(&pkg));
  REQUIRE(cat.packages().size() == 0);
}

TEST_CASE("empty category name", M) {
  try {
    Category cat{{}, nullptr};
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(e.what() == L("empty category name"));
  }
}

TEST_CASE("category full name", M) {
  Index ri(L("Remote Name"));
  Category cat2(L("Category Name"), &ri);
  REQUIRE(cat2.fullName() == L("Remote Name/Category Name"));
}

TEST_CASE("set index name", M) {
  SECTION("set") {
    Index ri({});
    ri.setName(L("Hello/World!"));
    REQUIRE(ri.name() == L("Hello/World!"));
  }

  SECTION("override") {
    Index ri(L("hello"));
    try {
      ri.setName(L("world"));
      FAIL();
    }
    catch(const reapack_error &e) {
      REQUIRE(e.what() == L("index name is already set"));
    }
    REQUIRE(ri.name() == L("hello"));
  }
}

TEST_CASE("find package", M) {
  Index ri(L("index name"));
  Category *cat = new Category(L("cat"), &ri);
  Package *pack = new Package(Package::ScriptType, L("pkg"), cat);
  Version *ver = new Version(L("1"), pack);
  Source *source = new Source({}, L("google.com"), ver);

  ver->addSource(source);
  pack->addVersion(ver);
  cat->addPackage(pack);
  ri.addCategory(cat);

  REQUIRE(ri.find(L("a"), L("b")) == nullptr);
  REQUIRE(ri.find(L("cat"), L("b")) == nullptr);
  REQUIRE(ri.find(L("cat"), L("pkg")) == pack);
}
