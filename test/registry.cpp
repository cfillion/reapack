#include <catch.hpp>

#include "helper/io.hpp"

#include <registry.hpp>

#include <errors.hpp>
#include <index.hpp>
#include <package.hpp>
#include <remote.hpp>

using namespace std;

static const char *M = "[registry]";

#define MAKE_PACKAGE \
  RemoteIndex ri("Remote Name"); \
  Category cat("Hello", &ri); \
  Package pkg(Package::ScriptType, "Hello", &cat); \
  Version *ver = new Version("1.0", &pkg); \
  Source *src = new Source(Source::GenericPlatform, "file", "url", ver); \
  ver->addSource(src); \
  pkg.addVersion(ver);

TEST_CASE("query uninstalled package", M) {
  MAKE_PACKAGE

  Registry reg;

  const Registry::Entry res = reg.query(&pkg);
  REQUIRE(res.status == Registry::Uninstalled);
  REQUIRE(res.version == 0);
}

TEST_CASE("query up to date pacakge", M) {
  MAKE_PACKAGE

  Registry reg;
  reg.push(ver);

  const Registry::Entry res = reg.query(&pkg);
  REQUIRE(res.status == Registry::UpToDate);
  REQUIRE(res.version == Version("1.0").code());
}

TEST_CASE("bump version", M) {
  MAKE_PACKAGE

  Version *ver2 = new Version("2.0", &pkg);
  ver2->addSource(new Source(Source::GenericPlatform, "file", "url", ver2));

  Registry reg;
  reg.push(ver);
  pkg.addVersion(ver2);

  const Registry::Entry res1 = reg.query(&pkg);
  REQUIRE(res1.status == Registry::UpdateAvailable);
  REQUIRE(res1.version == Version("1.0").code());

  reg.push(ver2);
  const Registry::Entry res2 = reg.query(&pkg);
  REQUIRE(res2.status == Registry::UpToDate);
  REQUIRE(res2.version == Version("2.0").code());
}

TEST_CASE("get file list", M) {
  MAKE_PACKAGE

  Registry reg;
  REQUIRE(reg.getFiles(reg.query(&pkg)).empty());

  reg.push(ver);

  const Registry::Entry res = reg.query(&pkg);
  const set<Path> files = reg.getFiles(res);

  REQUIRE(files == ver->files());
}

TEST_CASE("query all packages", M) {
  MAKE_PACKAGE

  const Remote remote("Remote Name", "irrelevent_url");

  Registry reg;
  REQUIRE(reg.queryAll(remote).empty());

  reg.push(ver);

  const vector<Registry::Entry> entries = reg.queryAll(remote);
  REQUIRE(entries.size() == 1);
  REQUIRE(entries[0].id == 1);
  REQUIRE(entries[0].status == Registry::Unknown);
  REQUIRE(entries[0].version == ver->code());
}

TEST_CASE("forget registry entry", M) {
  MAKE_PACKAGE

  Registry reg;
  reg.push(ver);

  reg.forget(reg.query(&pkg));

  const Registry::Entry afterForget = reg.query(&pkg);
  REQUIRE(afterForget.id == 0);
  REQUIRE(afterForget.status == Registry::Uninstalled);
  REQUIRE(afterForget.version == 0);
}

TEST_CASE("enforce unique files", M) {
  Registry reg;

  {
    MAKE_PACKAGE
    reg.push(ver);
  }

  RemoteIndex ri("Remote Name");
  Category cat("Hello", &ri);
  Package pkg(Package::ScriptType, "Duplicate Package", &cat);
  Version *ver = new Version("1.0", &pkg);
  Source *src = new Source(Source::GenericPlatform, "file", "url", ver);
  ver->addSource(src);
  pkg.addVersion(ver);

  try {
    reg.push(ver);
    FAIL("duplicate was accepted");
  }
  catch(const reapack_error &) {}
}
