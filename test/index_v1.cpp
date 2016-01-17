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
  try {
    RemoteIndex *ri = RemoteIndex::load("a", RIPATH "unnamed_category.xml");
    RIPTR(ri);
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "empty category name");
  }
}

TEST_CASE("invalid category tag", M) {
  RemoteIndex *ri = RemoteIndex::load("a", RIPATH "wrong_category_tag.xml");
  RIPTR(ri);

  REQUIRE(ri->categories().empty());
}

TEST_CASE("invalid package tag", M) {
  RemoteIndex *ri = RemoteIndex::load("a", RIPATH "wrong_package_tag.xml");
  RIPTR(ri);
  REQUIRE(ri->categories().empty());

}

TEST_CASE("null package name", M) {
  try {
    RemoteIndex *ri = RemoteIndex::load("a", RIPATH "unnamed_package.xml");
    RIPTR(ri);
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "empty package name");
  }
}

TEST_CASE("null package type", M) {
  try {
    RemoteIndex *ri = RemoteIndex::load("a", RIPATH "missing_type.xml");
    RIPTR(ri);
  }
  catch(const reapack_error &) {
    // no segfault -> test passes!
  }
}

TEST_CASE("invalid version tag", M) {
  RemoteIndex *ri = RemoteIndex::load("a", RIPATH "wrong_version_tag.xml");
  RIPTR(ri);

  REQUIRE(ri->categories().empty());
}

TEST_CASE("null package version", M) {
  try {
    RemoteIndex *ri = RemoteIndex::load("a", RIPATH "missing_version.xml");
    RIPTR(ri);
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "invalid version name");
  }
}

TEST_CASE("null source url", M) {
  try {
    RemoteIndex *ri = RemoteIndex::load("a", RIPATH "missing_source_url.xml");
    RIPTR(ri);
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "empty source url");
  }
}

TEST_CASE("null source file", M) {
  RemoteIndex *ri = RemoteIndex::load("a", RIPATH "missing_source_file.xml");
  RIPTR(ri);

  Package *pkg = ri->category(0)->package(0);
  REQUIRE(pkg->version(0)->source(0)->file() == pkg->name());
}

TEST_CASE("default platform", M) {
  RemoteIndex *ri = RemoteIndex::load("a", RIPATH "missing_platform.xml");
  RIPTR(ri);

  REQUIRE(ri->category(0)->package(0)->version(0)->source(0)->platform()
    == Source::GenericPlatform);
}

TEST_CASE("version changelog", M) {
  RemoteIndex *ri = RemoteIndex::load("a", RIPATH "changelog.xml");
  RIPTR(ri);

  CHECK_FALSE(ri->categories().empty());
  CHECK_FALSE(ri->category(0)->packages().empty());
  CHECK_FALSE(ri->category(0)->package(0)->versions().empty());

  REQUIRE(ri->category(0)->package(0)->version(0)->changelog()
    == "Hello\nWorld");
}

TEST_CASE("full index", M) {
  RemoteIndex *ri = RemoteIndex::load("a", RIPATH "valid_index.xml");
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
