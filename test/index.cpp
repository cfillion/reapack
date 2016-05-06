#include <catch.hpp>

#include <errors.hpp>
#include <index.hpp>

#include <string>

#define RIPATH "test/indexes/"

using namespace std;

static const char *M = "[index]";

TEST_CASE("index file not found", M) {
  UseRootPath root(RIPATH);

  try {
    IndexPtr ri = Index::load("404");
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "No such file or directory");
  }
}

TEST_CASE("load index from raw data", M) {
  SECTION("valid") {
    Index::load("", "<index version=\"1\"/>\n");
  }

  SECTION("broken") {
    try {
      Index::load("", "<index>\n");
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
    IndexPtr ri = Index::load("broken");
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "Error reading end tag.");
  }
}

TEST_CASE("wrong root tag name", M) {
  UseRootPath root(RIPATH);

  try {
    IndexPtr ri = Index::load("wrong_root");
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "invalid index");
  }
}

TEST_CASE("invalid version", M) {
  UseRootPath root(RIPATH);

  try {
    IndexPtr ri = Index::load("invalid_version");
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "index version not found");
  }
}

TEST_CASE("future version", M) {
  UseRootPath root(RIPATH);

  try {
    IndexPtr ri = Index::load("future_version");
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "index version is unsupported");
  }
}

TEST_CASE("unicode index path", M) {
  UseRootPath root(RIPATH);

  IndexPtr ri = Index::load("Новая папка");
  REQUIRE(ri->name() == "Новая папка");
}

TEST_CASE("add a category", M) {
  Index ri("a");
  Category *cat = new Category("a", &ri);
  Package *pack = new Package(Package::ScriptType, "name", cat);
  Version *ver = new Version("1", pack);
  ver->addSource(new Source({}, "google.com", ver));

  pack->addVersion(ver);
  cat->addPackage(pack);

  CHECK(ri.categories().size() == 0);
  CHECK(ri.category("a") == nullptr);

  ri.addCategory(cat);

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
    REQUIRE(string(e.what()) == "category belongs to another index");
  }
}

TEST_CASE("drop empty category", M) {
  Index ri("a");
  ri.addCategory(new Category("a", &ri));

  REQUIRE(ri.categories().empty());
}

TEST_CASE("add a package", M) {
  Index ri("a");
  Category cat("a", &ri);
  Package *pack = new Package(Package::ScriptType, "name", &cat);
  Version *ver = new Version("1", pack);
  ver->addSource(new Source({}, "google.com", ver));
  pack->addVersion(ver);

  CHECK(cat.packages().size() == 0);
  CHECK(cat.package("name") == nullptr);

  cat.addPackage(pack);

  REQUIRE(cat.packages().size() == 1);
  REQUIRE(cat.package("name") == pack);
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

  Index ri("Remote Name");
  Category cat2("Category Name", &ri);
  REQUIRE(cat2.fullName() == "Remote Name/Category Name");
}

TEST_CASE("repository description", M) {
  Index ri("Remote Name");
  CHECK(ri.aboutText().empty());

  ri.setAboutText("Hello World");
  REQUIRE(ri.aboutText() == "Hello World");
}

TEST_CASE("repository links", M) {
  Index ri("Remote name");
  CHECK(ri.links(Index::WebsiteLink).empty());
  CHECK(ri.links(Index::DonationLink).empty());

  SECTION("website links") {
    ri.addLink(Index::WebsiteLink, {"First", "http://example.com"});
    REQUIRE(ri.links(Index::WebsiteLink).size() == 1);
    ri.addLink(Index::WebsiteLink, {"Second", "http://example.com"});

    const auto &links = ri.links(Index::WebsiteLink);
    REQUIRE(links.size() == 2);
    REQUIRE(links[0]->name == "First");
    REQUIRE(links[1]->name == "Second");

    REQUIRE(ri.links(Index::DonationLink).empty());
  }

  SECTION("donation links") {
    ri.addLink(Index::DonationLink, {"First", "http://example.com"});
    REQUIRE(ri.links(Index::DonationLink).size() == 1);
  }

  SECTION("drop invalid links") {
    ri.addLink(Index::WebsiteLink, {"name", "not http(s)"});
    REQUIRE(ri.links(Index::WebsiteLink).empty());
  }
}

TEST_CASE("link type from string", M) {
  REQUIRE(Index::linkTypeFor("website") == Index::WebsiteLink);
  REQUIRE(Index::linkTypeFor("donation") == Index::DonationLink);
  REQUIRE(Index::linkTypeFor("bacon") == Index::WebsiteLink);
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
      REQUIRE(string(e.what()) == "index name is already set");
    }
    REQUIRE(ri.name() == "hello");
  }
}

TEST_CASE("find package from root", M) {
  const Path path("Category/Name/Package.lua");

  Index ri({});
  REQUIRE(ri.findPackage(path) == nullptr);

  Category *cat = new Category("Category/Name", &ri);

  {
    Package *pkg = new Package(Package::ScriptType, "Dummy.lua", cat);
    Version *ver = new Version("1", pkg);
    ver->addSource(new Source({}, "google.com", ver));
    pkg->addVersion(ver);
    cat->addPackage(pkg);
  }

  ri.addCategory(cat);
  REQUIRE(ri.findPackage(path) == nullptr);

  Package *pkg = new Package(Package::ScriptType, "Package.lua", cat);
  Version *ver = new Version("1", pkg);
  ver->addSource(new Source({}, "google.com", ver));
  pkg->addVersion(ver);
  cat->addPackage(pkg);

  REQUIRE(ri.findPackage(path) == pkg);
}
