#include "helper.hpp"

#include <errors.hpp>
#include <index.hpp>
#include <remote.hpp>

using namespace std;

static const char *M = "[index]";
static const Path RIPATH("test/indexes");

TEST_CASE("index file not found", M) {
  UseRootPath root(RIPATH);

  try {
    RemotePtr re = make_shared<Remote>("404", "url");
    Index ri(re);
    ri.load();
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "No such file or directory");
  }
}

TEST_CASE("load index from raw data", M) {
  Index ri;

  SECTION("valid") {
    ri.load("<index version=\"1\"/>\n");
  }

  SECTION("broken") {
    try {
      ri.load("<index>\n");
      FAIL();
    }
    catch(const reapack_error &e) {
      REQUIRE(string(e.what()) == "Error reading end tag.");
    }
  }
}

TEST_CASE("broken index", M) {
  UseRootPath root(RIPATH);

  try {
    RemotePtr re = make_shared<Remote>("broken", "url");
    Index ri(re);
    ri.load();
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "Error reading end tag.");
  }
}

TEST_CASE("wrong root tag name", M) {
  UseRootPath root(RIPATH);

  try {
    RemotePtr re = make_shared<Remote>("wrong_root", "url");
    Index ri(re);
    ri.load();
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "invalid index");
  }
}

TEST_CASE("invalid version", M) {
  UseRootPath root(RIPATH);

  try {
    RemotePtr re = make_shared<Remote>("invalid_version", "url");
    Index ri(re);
    ri.load();
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "index version not found");
  }
}

TEST_CASE("future version", M) {
  UseRootPath root(RIPATH);

  try {
    RemotePtr re = make_shared<Remote>("future_version", "url");
    Index ri(re);
    ri.load();
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "index version is unsupported");
  }
}

TEST_CASE("add a category", M) {
  RemotePtr re = make_shared<Remote>("Remote Name", "url");
  Index ri(re);
  Category *cat = new Category("a", &ri);
  Package *pack = new Package(Package::ScriptType, "name", cat);
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
  Index ri1;
  Index ri2;

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
  Index ri;
  const Category cat("a", &ri);
  REQUIRE_FALSE(ri.addCategory(&cat));

  REQUIRE(ri.categories().empty());
}

TEST_CASE("add a package", M) {
  RemotePtr re = make_shared<Remote>("Remote Name", "url");
  Index ri(re);
  Category cat("a", &ri);
  Package *pack = new Package(Package::ScriptType, "name", &cat);
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
  Package *pack = new Package(Package::ScriptType, "name", &cat1);

  try {
    Category cat2("b", nullptr);
    cat2.addPackage(pack);
    FAIL();
  }
  catch(const reapack_error &e) {
    delete pack;
    REQUIRE(string(e.what()) == "package belongs to another category");
  }
}

TEST_CASE("drop empty package", M) {
  Category cat("a", nullptr);
  const Package pkg(Package::ScriptType, "name", &cat);
  REQUIRE_FALSE(cat.addPackage(&pkg));
  REQUIRE(cat.packages().empty());
}

TEST_CASE("drop unknown package", M) {
  Category cat("a", nullptr);
  const Package pkg(Package::UnknownType, "name", &cat);
  REQUIRE_FALSE(cat.addPackage(&pkg));
  REQUIRE(cat.packages().size() == 0);
}

TEST_CASE("empty category name", M) {
  try {
    Category cat{{}, nullptr};
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "empty category name");
  }
}

TEST_CASE("category full name", M) {
  RemotePtr re = make_shared<Remote>("Remote Name", "url");
  Index ri(re);
  Category cat2("Category Name", &ri);
  REQUIRE(cat2.fullName() == "Remote Name/Category Name");
}

TEST_CASE("find package", M) {
  RemotePtr re = make_shared<Remote>("Remote Name", "url");
  Index ri(re);
  Category *cat = new Category("cat", &ri);
  Package *pack = new Package(Package::ScriptType, "pkg", cat);
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
