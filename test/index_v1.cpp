#include <catch.hpp>

#include <index.hpp>
#include <errors.hpp>

#include <memory>
#include <string>

#define RIPATH "test/indexes/v1/"
#define RIPTR(ptr) unique_ptr<RemoteIndex> riptr(ptr)

using namespace std;

static const char *M = "[reapack_v1]";

TEST_CASE("unnamed category", M) {
  UseRootPath root(RIPATH);

  try {
    RemoteIndex *ri = RemoteIndex::load("unnamed_category");
    RIPTR(ri);
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "empty category name");
  }
}

TEST_CASE("invalid category tag", M) {
  UseRootPath root(RIPATH);

  RemoteIndex *ri = RemoteIndex::load("wrong_category_tag");
  RIPTR(ri);

  REQUIRE(ri->categories().empty());
}

TEST_CASE("invalid package tag", M) {
  UseRootPath root(RIPATH);

  RemoteIndex *ri = RemoteIndex::load("wrong_package_tag");
  RIPTR(ri);
  REQUIRE(ri->categories().empty());

}

TEST_CASE("null package name", M) {
  UseRootPath root(RIPATH);

  try {
    RemoteIndex *ri = RemoteIndex::load("unnamed_package");
    RIPTR(ri);
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "empty package name");
  }
}

TEST_CASE("null package type", M) {
  UseRootPath root(RIPATH);

  try {
    RemoteIndex *ri = RemoteIndex::load("missing_type");
    RIPTR(ri);
  }
  catch(const reapack_error &) {
    // no segfault -> test passes!
  }
}

TEST_CASE("read version author", M) {
  UseRootPath root(RIPATH);

  RemoteIndex *ri = RemoteIndex::load("author");
  RIPTR(ri);

  CHECK(ri->packages().size() == 1);
  REQUIRE(ri->category(0)->package(0)->lastVersion()->author()
    == "Watanabe Saki");
}

TEST_CASE("invalid version tag", M) {
  UseRootPath root(RIPATH);

  RemoteIndex *ri = RemoteIndex::load("wrong_version_tag");
  RIPTR(ri);

  REQUIRE(ri->categories().empty());
}

TEST_CASE("null package version", M) {
  UseRootPath root(RIPATH);

  try {
    RemoteIndex *ri = RemoteIndex::load("missing_version");
    RIPTR(ri);
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "invalid version name");
  }
}

TEST_CASE("null source url", M) {
  UseRootPath root(RIPATH);

  try {
    RemoteIndex *ri = RemoteIndex::load("missing_source_url");
    RIPTR(ri);
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "empty source url");
  }
}

TEST_CASE("null source file", M) {
  UseRootPath root(RIPATH);

  RemoteIndex *ri = RemoteIndex::load("missing_source_file");
  RIPTR(ri);

  CHECK(ri->packages().size() == 1);

  Package *pkg = ri->category(0)->package(0);
  REQUIRE(pkg->version(0)->source(0)->file() == pkg->name());
}

TEST_CASE("default platform", M) {
  UseRootPath root(RIPATH);

  RemoteIndex *ri = RemoteIndex::load("missing_platform");
  RIPTR(ri);

  CHECK(ri->packages().size() == 1);
  REQUIRE(ri->category(0)->package(0)->version(0)->source(0)->platform()
    == Source::GenericPlatform);
}

TEST_CASE("version changelog", M) {
  UseRootPath root(RIPATH);

  RemoteIndex *ri = RemoteIndex::load("changelog");
  RIPTR(ri);

  CHECK(ri->packages().size() == 1);

  REQUIRE(ri->category(0)->package(0)->version(0)->changelog()
    == "Hello\nWorld");
}

TEST_CASE("full index", M) {
  UseRootPath root(RIPATH);

  RemoteIndex *ri = RemoteIndex::load("valid_index");
  RIPTR(ri);

  REQUIRE(ri->categories().size() == 1);

  Category *cat = ri->category(0);
  REQUIRE(cat->name() == "Category Name");
  REQUIRE(cat->packages().size() == 1);

  Package *pack = cat->package(0);
  REQUIRE(pack->type() == Package::ScriptType);
  REQUIRE(pack->name() == "Hello World.lua");
  REQUIRE(pack->versions().size() == 1);

  Version *ver = pack->version(0);
  REQUIRE(ver->name() == "1.0");
  REQUIRE(ver->sources().size() == 1);
  REQUIRE(ver->changelog() == "Fixed a division by zero error.");

  Source *source = ver->source(0);
  REQUIRE(source->platform() == Source::GenericPlatform);
  REQUIRE(source->file() == "test.lua");
  REQUIRE(source->url() == "https://google.com/");
}
