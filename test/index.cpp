#include <catch.hpp>

#include <errors.hpp>
#include <index.hpp>

#include <string>

#define RIPATH "test/indexes/"
#define RIPTR(ptr) unique_ptr<RemoteIndex> riptr(ptr)

using namespace std;

static const char *M = "[index]";

TEST_CASE("file not found", M) {
  UseRootPath root(RIPATH);

  try {
    RemoteIndex *ri = RemoteIndex::load("404");
    RIPTR(ri);
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "No such file or directory");
  }
}

TEST_CASE("broken", M) {
  UseRootPath root(RIPATH);

  try {
    RemoteIndex *ri = RemoteIndex::load("broken");
    RIPTR(ri);
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "Error reading end tag.");
  }
}

TEST_CASE("wrong root tag name", M) {
  UseRootPath root(RIPATH);

  try {
    RemoteIndex *ri = RemoteIndex::load("wrong_root");
    RIPTR(ri);
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "invalid index");
  }
}

TEST_CASE("invalid version", M) {
  UseRootPath root(RIPATH);

  try {
    RemoteIndex *ri = RemoteIndex::load("invalid_version");
    RIPTR(ri);
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "invalid version");
  }
}

TEST_CASE("future version", M) {
  UseRootPath root(RIPATH);

  try {
    RemoteIndex *ri = RemoteIndex::load("future_version");
    RIPTR(ri);
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "unsupported version");
  }
}

TEST_CASE("unicode index path", M) {
  UseRootPath root(RIPATH);

  RemoteIndex *ri = RemoteIndex::load("Новая папка");
  RIPTR(ri);

  REQUIRE(ri->name() == "Новая папка");
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

TEST_CASE("repository description", M) {
  RemoteIndex ri("Remote Name");
  CHECK(ri.aboutText().empty());

  ri.setAboutText("Hello World");
  REQUIRE(ri.aboutText() == "Hello World");
}

TEST_CASE("repository links", M) {
  RemoteIndex ri("Remote name");
  CHECK(ri.links(RemoteIndex::WebsiteLink).empty());
  CHECK(ri.links(RemoteIndex::DonationLink).empty());

  SECTION("website links") {
    ri.addLink(RemoteIndex::WebsiteLink, {"First", "http://example.com"});
    REQUIRE(ri.links(RemoteIndex::WebsiteLink).size() == 1);
    ri.addLink(RemoteIndex::WebsiteLink, {"Second", "http://example.com"});

    const auto &links = ri.links(RemoteIndex::WebsiteLink);
    REQUIRE(links.size() == 2);
    REQUIRE(links[0]->name == "First");
    REQUIRE(links[1]->name == "Second");

    REQUIRE(ri.links(RemoteIndex::DonationLink).empty());
  }

  SECTION("donation links") {
    ri.addLink(RemoteIndex::DonationLink, {"First", "http://example.com"});
    REQUIRE(ri.links(RemoteIndex::DonationLink).size() == 1);
  }

  SECTION("drop invalid links") {
    ri.addLink(RemoteIndex::WebsiteLink, {"name", "not http(s)"});
    REQUIRE(ri.links(RemoteIndex::WebsiteLink).empty());
  }
}

TEST_CASE("link type from string", M) {
  REQUIRE(RemoteIndex::linkTypeFor("website") == RemoteIndex::WebsiteLink);
  REQUIRE(RemoteIndex::linkTypeFor("donation") == RemoteIndex::DonationLink);
  REQUIRE(RemoteIndex::linkTypeFor("bacon") == RemoteIndex::WebsiteLink);
}
