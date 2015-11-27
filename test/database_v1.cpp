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
  try {
    Database::load(DBPATH "wrong_package_tag.xml");
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "empty category");
  }
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
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "unsupported package type");
  }
}

TEST_CASE("null author", M) {
  try {
    Database::load(DBPATH "anonymous_package.xml");
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "empty package author");
  }
}

TEST_CASE("invalid version tag", M) {
  try {
    Database::load(DBPATH "wrong_version_tag.xml");
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "empty package");
  }
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

