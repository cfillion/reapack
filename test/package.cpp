#include <catch.hpp>

#include <database.hpp>
#include <errors.hpp>
#include <package.hpp>

#include <string>

using namespace std;

static const char *M = "[package]";

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
  Package pack(Package::ScriptType, "a");
  CHECK(pack.versions().size() == 0);

  SourcePtr source = make_shared<Source>(Source::GenericPlatform, "google.com");

  VersionPtr final = make_shared<Version>("1");
  final->addSource(source);

  VersionPtr alpha = make_shared<Version>("0.1");
  alpha->addSource(source);

  pack.addVersion(final);
  CHECK(pack.versions().size() == 1);

  pack.addVersion(alpha);
  CHECK(pack.versions().size() == 2);

  REQUIRE(pack.version(0) == alpha);
  REQUIRE(pack.version(1) == final);
  REQUIRE(pack.lastVersion() == final);
}

TEST_CASE("drop empty version", M) {
  Package pack(Package::ScriptType, "a");
  pack.addVersion(make_shared<Version>("1"));

  REQUIRE(pack.versions().empty());
}

TEST_CASE("different install location", M) {
  const InstallLocation a{"/hello", "/world"};
  const InstallLocation b{"/chunky", "/bacon"};

  REQUIRE_FALSE(a == b);
}

TEST_CASE("set install location prefix", M) {
  InstallLocation loc{"/hello", "world"};

  REQUIRE(loc.directory() == "/hello");
  REQUIRE(loc.filename() == "world");
  REQUIRE(loc.fullPath() == "/hello/world");

  loc.prependDir("/root");

  REQUIRE(loc.directory() == "/root/hello");
  REQUIRE(loc.fullPath() == "/root/hello/world");

  loc.appendDir("/to");

  REQUIRE(loc.directory() == "/root/hello/to");
  REQUIRE(loc.filename() == "world");
  REQUIRE(loc.fullPath() == "/root/hello/to/world");
}

TEST_CASE("unknown target location", M) {
  Package pack(Package::UnknownType, "a");

  try {
    pack.targetLocation();
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "unsupported package type");
  }
}

TEST_CASE("script target location", M) {
  Category cat("Category Name");

  Package pack(Package::ScriptType, "file.name");
  pack.setCategory(&cat);

  const InstallLocation loc = pack.targetLocation();
  REQUIRE(loc ==
    InstallLocation("/Scripts/ReaScripts/Category Name", "file.name"));
}

TEST_CASE("script target location without category", M) {
  Package pack(Package::ScriptType, "file.name");

  const InstallLocation loc = pack.targetLocation();
  REQUIRE(loc ==
    InstallLocation("/Scripts/ReaScripts", "file.name"));
}
