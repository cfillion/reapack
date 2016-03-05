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
  Category cat("Category Name", &ri); \
  Package pkg(Package::ScriptType, "Hello", &cat); \
  Version *ver = new Version("1.0", &pkg); \
  Source *src = new Source(Source::GenericPlatform, "file", "url", ver); \
  ver->addSource(src); \
  pkg.addVersion(ver);

TEST_CASE("query uninstalled package", M) {
  MAKE_PACKAGE

  Registry reg;

  const auto &res = reg.query(&pkg);
  REQUIRE(res.status == Registry::Uninstalled);
  REQUIRE(res.entry.id == 0);
  REQUIRE(res.entry.version == 0);
}

TEST_CASE("query up to date pacakge", M) {
  MAKE_PACKAGE

  Registry reg;

  const Registry::Entry &entry = reg.push(ver);
  REQUIRE(entry.id == 1);
  REQUIRE(entry.remote == "Remote Name");
  REQUIRE(entry.category == "Category Name");
  REQUIRE(entry.package == "Hello");
  REQUIRE(entry.type == Package::ScriptType);
  REQUIRE(entry.version == Version("1.0").code());

  const Registry::QueryResult &queryRes = reg.query(&pkg);
  REQUIRE(queryRes.status == Registry::UpToDate);
  REQUIRE(queryRes.entry.version == Version("1.0").code());
}

TEST_CASE("bump version", M) {
  MAKE_PACKAGE

  Version *ver2 = new Version("2.0", &pkg);
  ver2->addSource(new Source(Source::GenericPlatform, "file", "url", ver2));

  Registry reg;
  reg.push(ver);
  pkg.addVersion(ver2);

  const Registry::QueryResult &res1 = reg.query(&pkg);
  REQUIRE(res1.status == Registry::UpdateAvailable);
  REQUIRE(res1.entry.version == Version("1.0").code());

  reg.push(ver2);
  const Registry::QueryResult &res2 = reg.query(&pkg);
  REQUIRE(res2.status == Registry::UpToDate);
  REQUIRE(res2.entry.version == Version("2.0").code());
  
  REQUIRE(res2.entry.id == res1.entry.id);
}

TEST_CASE("get file list", M) {
  MAKE_PACKAGE

  Registry reg;
  REQUIRE(reg.getFiles(reg.query(&pkg).entry).empty());

  reg.push(ver);

  const set<Path> &files = reg.getFiles(reg.query(&pkg).entry);

  REQUIRE(files == ver->files());
}

TEST_CASE("query all packages", M) {
  MAKE_PACKAGE

  const string remote = "Remote Name";

  Registry reg;
  REQUIRE(reg.getEntries(remote).empty());

  reg.push(ver);

  const vector<Registry::Entry> &entries = reg.getEntries(remote);
  REQUIRE(entries.size() == 1);
  REQUIRE(entries[0].id == 1);
  REQUIRE(entries[0].remote == "Remote Name");
  REQUIRE(entries[0].category == "Category Name");
  REQUIRE(entries[0].package == "Hello");
  REQUIRE(entries[0].type == Package::ScriptType);
  REQUIRE(entries[0].version == Version("1.0").code());
}

TEST_CASE("forget registry entry", M) {
  MAKE_PACKAGE

  Registry reg;
  reg.forget(reg.push(ver));

  const Registry::QueryResult &afterForget = reg.query(&pkg);
  REQUIRE(afterForget.status == Registry::Uninstalled);
  REQUIRE(afterForget.entry.id == 0);
}

TEST_CASE("file conflicts", M) {
  Registry reg;

  {
    MAKE_PACKAGE
    reg.push(ver);
  }

  RemoteIndex ri("Remote Name");
  Category cat("Category Name", &ri);
  Package pkg(Package::ScriptType, "Duplicate Package", &cat);
  Version *ver = new Version("1.0", &pkg);
  Source *src1 = new Source(Source::GenericPlatform, "file", "url", ver);
  Source *src2 = new Source(Source::GenericPlatform, "file2", "url", ver);
  ver->addSource(src1);
  ver->addSource(src2);
  pkg.addVersion(ver);

  CHECK(reg.query(&pkg).status == Registry::Uninstalled);

  try {
    reg.push(ver);
    FAIL("duplicate is accepted");
  }
  catch(const reapack_error &) {}

  REQUIRE(reg.query(&pkg).status == Registry::Uninstalled);

  vector<Path> conflicts;
  reg.push(ver, &conflicts);

  REQUIRE(conflicts.size() == 1);
  REQUIRE(conflicts[0] == src1->targetPath());

  REQUIRE(reg.query(&pkg).status == Registry::Uninstalled);
}

TEST_CASE("get main file", M) {
  MAKE_PACKAGE

  Registry reg;
  REQUIRE((reg.getMainFile({})).empty());

  Source *main = new Source(Source::GenericPlatform, {}, "url", ver);
  ver->addSource(main);

  const Registry::Entry &entry = reg.push(ver);
  REQUIRE(reg.getMainFile(entry) == main->targetPath().join('/'));
}
