#include <catch.hpp>

#include "helper/io.hpp"

#include <errors.hpp>
#include <index.hpp>
#include <package.hpp>

#include <string>

using namespace std;

static const char *M = "[package]";

TEST_CASE("package type from string", M) {
  SECTION("unknown")
    REQUIRE(Package::typeFor("yoyo") == Package::UnknownType);

  SECTION("script")
    REQUIRE(Package::typeFor("script") == Package::ScriptType);

  SECTION("extension")
    REQUIRE(Package::typeFor("extension") == Package::ExtensionType);

  SECTION("effect")
    REQUIRE(Package::typeFor("effect") == Package::EffectType);

  SECTION("data")
    REQUIRE(Package::typeFor("data") == Package::DataType);
}

TEST_CASE("package type to string", M) {
  SECTION("unknown") {
    REQUIRE("Unknown" == Package::displayType(Package::UnknownType));
    REQUIRE("Unknown" == Package(Package::UnknownType, "test").displayType());
  }

  SECTION("script") {
    REQUIRE("Script" == Package::displayType(Package::ScriptType));
    REQUIRE("Script" == Package(Package::ScriptType, "test").displayType());
  }

  SECTION("extension") {
    REQUIRE("Extension" == Package::displayType(Package::ExtensionType));
    REQUIRE("Extension" == Package(Package::ExtensionType, "test").displayType());
  }

  SECTION("effect") {
    REQUIRE("Effect" == Package::displayType(Package::EffectType));
    REQUIRE("Effect" == Package(Package::EffectType, "test").displayType());
  }

  SECTION("data") {
    REQUIRE("Data" == Package::displayType(Package::DataType));
    REQUIRE("Data" == Package(Package::DataType, "test").displayType());
  }

  SECTION("unknown value")
    REQUIRE("Unknown" == Package::displayType(static_cast<Package::Type>(-1)));
}

TEST_CASE("empty package name", M) {
  try {
    Package pack(Package::ScriptType, string());
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "empty package name");
  }
}

TEST_CASE("package versions are sorted", M) {
  Index ri("Remote Name");
  Category cat("Category Name", &ri);

  Package pack(Package::ScriptType, "a", &cat);
  CHECK(pack.versions().size() == 0);

  Version *final = new Version("1", &pack);
  final->addSource(new Source(Source::GenericPlatform, {}, "google.com", final));

  Version *alpha = new Version("0.1", &pack);
  alpha->addSource(new Source(Source::GenericPlatform, {}, "google.com", alpha));

  pack.addVersion(final);
  REQUIRE(final->package() == &pack);
  CHECK(pack.versions().size() == 1);

  pack.addVersion(alpha);
  CHECK(pack.versions().size() == 2);

  REQUIRE(pack.version(0) == alpha);
  REQUIRE(pack.version(1) == final);
  REQUIRE(pack.lastVersion() == final);
}

TEST_CASE("drop empty version", M) {
  Package pack(Package::ScriptType, "a");
  pack.addVersion(new Version("1", &pack));

  REQUIRE(pack.versions().empty());
  REQUIRE(pack.lastVersion() == nullptr);
}

TEST_CASE("add owned version", M) {
  Package pack1(Package::ScriptType, "a");
  Package pack2(Package::ScriptType, "a");

  Version *ver = new Version("1", &pack1);

  try {
    pack2.addVersion(ver);
    FAIL();
  }
  catch(const reapack_error &e) {
    delete ver;
    REQUIRE(string(e.what()) == "version belongs to another package");
  }
}

TEST_CASE("find matching version", M) {
  Index ri("Remote Name");
  Category cat("Category Name", &ri);

  Package pack(Package::ScriptType, "a", &cat);
  CHECK(pack.versions().size() == 0);

  Version *ver = new Version("1", &pack);
  ver->addSource(new Source(Source::GenericPlatform, {}, "google.com", ver));

  REQUIRE(pack.findVersion(Version("1")) == nullptr);
  REQUIRE(pack.findVersion(Version("2")) == nullptr);

  pack.addVersion(ver);

  REQUIRE(pack.findVersion(Version("1")) == ver);
  REQUIRE(pack.findVersion(Version("2")) == nullptr);
}

TEST_CASE("target path for unknown package type", M) {
  Index ri("name");
  Category cat("name", &ri);

  Package pack(Package::UnknownType, "a", &cat);

  REQUIRE(pack.makeTargetPath({}).empty());
}

TEST_CASE("script target path", M) {
  Index ri("Remote Name");
  Category cat("Category Name", &ri);

  Package pack(Package::ScriptType, "file.name", &cat);

  Path expected;
  expected.append("Scripts");
  expected.append("Remote Name");
  expected.append("Category Name");
  REQUIRE(pack.makeTargetPath() == expected);

  expected.removeLast();
  expected.append("hello_world.lua");
  REQUIRE(pack.makeTargetPath("../../../hello_world.lua") == expected);
}

TEST_CASE("limited directory traversal from category", M) {
  Index ri("Remote Name");
  Category cat("../..", &ri);

  Package pack(Package::ScriptType, "file.name", &cat);

  Path expected;
  expected.append("Scripts");
  expected.append("Remote Name");

  REQUIRE(pack.makeTargetPath() == expected);
}

TEST_CASE("target path without category", M) {
  Package pack(Package::ScriptType, "file.name");

  try {
    pack.makeTargetPath();
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "category or index is unset");
  }
}

TEST_CASE("extension target path", M) {
  Index ri("Remote Name");
  Category cat("Category Name", &ri);

  Package pack(Package::ExtensionType, "file.name", &cat);

  Path expected;
  expected.append("UserPlugins");
  REQUIRE(pack.makeTargetPath() == expected);

  expected.append("reaper_reapack.dll");
  REQUIRE(pack.makeTargetPath("../reaper_reapack.dll") == expected);
}

TEST_CASE("effect target path", M) {
  Index ri("Remote Name");
  Category cat("Category Name", &ri);

  Package pack(Package::EffectType, "file.name", &cat);

  Path expected;
  expected.append("Effects");
  expected.append("Remote Name");
  expected.append("Category Name");

  REQUIRE(pack.makeTargetPath() == expected);

  expected.removeLast();
  expected.append("hello_world.jsfx");
  REQUIRE(pack.makeTargetPath("../../../hello_world.jsfx") == expected);
}

TEST_CASE("data target path", M) {
  Index ri("Remote Name");
  Category cat("Category Name", &ri);

  Package pack(Package::DataType, "file.name", &cat);

  Path expected;
  expected.append("Data");
  expected.append("Remote Name");
  expected.append("Category Name");

  REQUIRE(pack.makeTargetPath() == expected);

  expected.removeLast();
  expected.append("hello_world.jsfx");
  REQUIRE(pack.makeTargetPath("../../../hello_world.jsfx") == expected);
}

TEST_CASE("full name", M) {
  SECTION("no category") {
    Package pack(Package::ScriptType, "file.name");
    REQUIRE(pack.fullName() == "file.name");
  }

  SECTION("with category") {
    Category cat("Category Name");
    Package pack(Package::ScriptType, "file.name", &cat);
    REQUIRE(pack.fullName() == "Category Name/file.name");
  }

  SECTION("with index") {
    Index ri("Remote Name");
    Category cat("Category Name", &ri);
    Package pack(Package::ScriptType, "file.name", &cat);

    REQUIRE(pack.fullName() == "Remote Name/Category Name/file.name");
  }
}
