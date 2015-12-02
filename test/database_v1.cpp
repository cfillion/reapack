#include <catch.hpp>

#include <database.hpp>
#include <errors.hpp>

#include <string>

#define DBPATH "test/db/v1/"

using namespace std;

static const char *M = "[reapack_v1]";

TEST_CASE("unnamed category", M) {
  try {
    Database::load(DBPATH "unnamed_category.xml");
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "empty category name");
  }
}

TEST_CASE("invalid category tag", M) {
  DatabasePtr db = Database::load(DBPATH "wrong_category_tag.xml");
  REQUIRE(db->categories().empty());
}

TEST_CASE("invalid package tag", M) {
  DatabasePtr db = Database::load(DBPATH "wrong_package_tag.xml");
  REQUIRE(db->categories().empty());

}

TEST_CASE("null package name", M) {
  try {
    Database::load(DBPATH "unnamed_package.xml");
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "empty package name");
  }
}

TEST_CASE("null package type", M) {
  try {
    Database::load(DBPATH "missing_type.xml");
  }
  catch(const reapack_error &e) {
    // no segfault -> test passes!
  }
}

TEST_CASE("invalid version tag", M) {
  DatabasePtr db = Database::load(DBPATH "wrong_version_tag.xml");

  REQUIRE(db->categories().empty());
}

TEST_CASE("null package version", M) {
  try {
    Database::load(DBPATH "missing_version.xml");
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "invalid version name");
  }
}

TEST_CASE("null source url", M) {
  try {
    Database::load(DBPATH "missing_source_url.xml");
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "empty source url");
  }
}

TEST_CASE("default platform", M) {
  DatabasePtr db = Database::load(DBPATH "missing_platform.xml");

  REQUIRE(db->category(0)->package(0)->version(0)->source(0)->platform()
    == Source::GenericPlatform);
}

TEST_CASE("version changelog", M) {
  DatabasePtr db = Database::load(DBPATH "changelog.xml");

  CHECK_FALSE(db->categories().empty());
  CHECK_FALSE(db->category(0)->packages().empty());
  CHECK_FALSE(db->category(0)->package(0)->versions().empty());

  REQUIRE(db->category(0)->package(0)->version(0)->changelog()
    == "Hello\nWorld");
}

TEST_CASE("full database", M) {
  DatabasePtr db = Database::load(DBPATH "full_database.xml");
  REQUIRE(db->categories().size() == 1);

  CategoryPtr cat = db->category(0);
  REQUIRE(cat->name() == "Category Name");
  REQUIRE(cat->packages().size() == 1);

  PackagePtr pack = cat->package(0);
  REQUIRE(pack->type() == Package::ScriptType);
  REQUIRE(pack->name() == "Hello World.lua");
  REQUIRE(pack->versions().size() == 1);

  VersionPtr ver = pack->version(0);
  REQUIRE(ver->name() == "1.0");
  REQUIRE(ver->sources().size() == 1);
  REQUIRE(ver->changelog() == "Fixed a division by zero error.");

  SourcePtr source = ver->source(0);
  REQUIRE(source->platform() == Source::GenericPlatform);
  REQUIRE(source->url() == "https://google.com/");
}
