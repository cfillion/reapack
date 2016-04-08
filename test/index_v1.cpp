#include <catch.hpp>

#include <index.hpp>
#include <errors.hpp>

#include <string>

#define RIPATH "test/indexes/v1/"
#define REMOTE(x) Remote{x, "url"}

using namespace std;

static const char *M = "[reapack_v1]";

TEST_CASE("unnamed category", M) {
  UseRootPath root(RIPATH);

  try {
    IndexPtr ri = Index::load(REMOTE("unnamed_category"));
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "empty category name");
  }
}

TEST_CASE("invalid category tag", M) {
  UseRootPath root(RIPATH);

  IndexPtr ri = Index::load(REMOTE("wrong_category_tag"));

  REQUIRE(ri->categories().empty());
}

TEST_CASE("invalid package tag", M) {
  UseRootPath root(RIPATH);

  IndexPtr ri = Index::load(REMOTE("wrong_package_tag"));
  REQUIRE(ri->categories().empty());

}

TEST_CASE("null package name", M) {
  UseRootPath root(RIPATH);

  try {
    IndexPtr ri = Index::load(REMOTE("unnamed_package"));
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "empty package name");
  }
}

TEST_CASE("null package type", M) {
  UseRootPath root(RIPATH);

  try {
    IndexPtr ri = Index::load(REMOTE("missing_type"));
  }
  catch(const reapack_error &) {
    // no segfault -> test passes!
  }
}

TEST_CASE("unsupported package type", M) {
  UseRootPath root(RIPATH);

  IndexPtr ri = Index::load(REMOTE("unsupported_type"));
}

TEST_CASE("read version author", M) {
  UseRootPath root(RIPATH);

  IndexPtr ri = Index::load(REMOTE("author"));

  CHECK(ri->packages().size() == 1);
  REQUIRE(ri->category(0)->package(0)->lastVersion()->author()
    == "Watanabe Saki");
}

TEST_CASE("read version time", M) {
  UseRootPath root(RIPATH);

  IndexPtr ri = Index::load(REMOTE("time"));
  CHECK(ri->packages().size() == 1);

  const tm &time = ri->category(0)->package(0)->lastVersion()->time();
  REQUIRE(time.tm_year == 2016 - 1900);
  REQUIRE(time.tm_mon == 2 - 1);
  REQUIRE(time.tm_mday == 12);
}

TEST_CASE("invalid version tag", M) {
  UseRootPath root(RIPATH);

  IndexPtr ri = Index::load(REMOTE("wrong_version_tag"));
  REQUIRE(ri->categories().empty());
}

TEST_CASE("null package version", M) {
  UseRootPath root(RIPATH);

  try {
    IndexPtr ri = Index::load(REMOTE("missing_version"));
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "invalid version name");
  }
}

TEST_CASE("null source url", M) {
  UseRootPath root(RIPATH);

  try {
    IndexPtr ri = Index::load(REMOTE("missing_source_url"));
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "empty source url");
  }
}

TEST_CASE("null source file", M) {
  UseRootPath root(RIPATH);

  IndexPtr ri = Index::load(REMOTE("missing_source_file"));
  CHECK(ri->packages().size() == 1);

  const Package *pkg = ri->category(0)->package(0);
  REQUIRE(pkg->version(0)->source(0)->file() == pkg->name());
}

TEST_CASE("default platform", M) {
  UseRootPath root(RIPATH);

  IndexPtr ri = Index::load(REMOTE("missing_platform"));

  CHECK(ri->packages().size() == 1);
  REQUIRE(ri->category(0)->package(0)->version(0)->source(0)->platform()
    == Source::GenericPlatform);
}

TEST_CASE("version changelog", M) {
  UseRootPath root(RIPATH);

  IndexPtr ri = Index::load(REMOTE("changelog"));
  CHECK(ri->packages().size() == 1);

  REQUIRE(ri->category(0)->package(0)->version(0)->changelog()
    == "Hello\nWorld");
}

TEST_CASE("full index", M) {
  UseRootPath root(RIPATH);

  IndexPtr ri = Index::load(REMOTE("valid_index"));
  REQUIRE(ri->categories().size() == 1);

  const Category *cat = ri->category(0);
  REQUIRE(cat->name() == "Category Name");
  REQUIRE(cat->packages().size() == 1);

  const Package *pack = cat->package(0);
  REQUIRE(pack->type() == Package::ScriptType);
  REQUIRE(pack->name() == "Hello World.lua");
  REQUIRE(pack->versions().size() == 1);

  const Version *ver = pack->version(0);
  REQUIRE(ver->name() == "1.0");
  REQUIRE(ver->sources().size() == 1);
  REQUIRE(ver->changelog() == "Fixed a division by zero error.");

  const Source *source = ver->source(0);
  REQUIRE(source->platform() == Source::GenericPlatform);
  REQUIRE(source->file() == "test.lua");
  REQUIRE(source->url() == "https://google.com/");
}

TEST_CASE("read index metadata", M) {
  UseRootPath root(RIPATH);

  IndexPtr ri = Index::load(REMOTE("metadata"));
  
  SECTION("name (ignored)") {
    REQUIRE(ri->name() == "metadata");
  }

  SECTION("description") {
    REQUIRE(ri->aboutText() == "Chunky\nBacon");
  }
  
  SECTION("website links") {
    const auto &links = ri->links(Index::WebsiteLink);
    REQUIRE(links.size() == 4);
    REQUIRE(links[0]->name == "http://cfillion.tk");
    REQUIRE(links[0]->url == "http://cfillion.tk");
    REQUIRE(links[1]->name == "https://github.com/cfillion");
    REQUIRE(links[1]->url == "https://github.com/cfillion");
    REQUIRE(links[2]->name == "http://twitter.com/cfi30");
    REQUIRE(links[2]->url == "http://twitter.com/cfi30");
    REQUIRE(links[3]->name == "http://google.com");
    REQUIRE(links[3]->url == "http://google.com");
  }

  SECTION("donation links") {
    const auto &links = ri->links(Index::DonationLink);
    REQUIRE(links.size() == 1);
    REQUIRE(links[0]->name == "Donate");
    REQUIRE(links[0]->url == "http://paypal.com");
  }
}

TEST_CASE("read index name (from raw data only)") {
  SECTION("valid") {
    IndexPtr ri = Index::load({}, "<index version=\"1\" name=\"Hello World\"/>\n");
    REQUIRE(ri->name() == "Hello World");
  }

  SECTION("missing") {
    IndexPtr ri = Index::load({}, "<index version=\"1\"/>\n");
    REQUIRE(ri->name() == "");
  }
}
