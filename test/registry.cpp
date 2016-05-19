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
  Index ri("Remote Name"); \
  Category cat("Category Name", &ri); \
  Package pkg(Package::ScriptType, "Hello", &cat); \
  Version *ver = new Version("1.0", &pkg); \
  ver->setAuthor("John Doe"); \
  Source *src = new Source("file", "url", ver); \
  ver->addSource(src); \
  pkg.addVersion(ver);

TEST_CASE("query uninstalled package", M) {
  MAKE_PACKAGE

  Registry reg;

  const Registry::Entry &entry = reg.getEntry(&pkg);
  REQUIRE_FALSE(entry);
  REQUIRE(entry.id == 0);
  REQUIRE(entry.version == Version());
}

TEST_CASE("query installed package", M) {
  MAKE_PACKAGE

  Registry reg;

  const Registry::Entry &entry = reg.push(ver);
  REQUIRE(entry);
  REQUIRE(entry.id == 1);
  REQUIRE(entry.remote == "Remote Name");
  REQUIRE(entry.category == "Category Name");
  REQUIRE(entry.package == "Hello");
  REQUIRE(entry.type == Package::ScriptType);
  REQUIRE(entry.version.name() == "1.0");
  REQUIRE(entry.version.author() == "John Doe");

  const Registry::Entry &selectEntry = reg.getEntry(&pkg);
  REQUIRE(selectEntry.id == entry.id);
  REQUIRE(selectEntry.remote == entry.remote);
  REQUIRE(selectEntry.category == entry.category);
  REQUIRE(selectEntry.package == entry.package);
  REQUIRE(selectEntry.type == entry.type);
  REQUIRE(selectEntry.version == entry.version);
  REQUIRE(selectEntry.version.author() == entry.version.author());
}

TEST_CASE("bump version", M) {
  MAKE_PACKAGE

  Version *ver2 = new Version("2.0", &pkg);
  ver2->addSource(new Source("file", "url", ver2));

  Registry reg;
  reg.push(ver);
  pkg.addVersion(ver2);

  const Registry::Entry &entry1 = reg.getEntry(&pkg);
  REQUIRE(entry1.version.name() == "1.0");
  CHECK(entry1.version.author() == "John Doe");

  reg.push(ver2);
  const Registry::Entry &entry2 = reg.getEntry(&pkg);
  REQUIRE(entry2.version.name() == "2.0");
  CHECK(entry2.version.author() == "");
  
  REQUIRE(entry2.id == entry1.id);
}

TEST_CASE("get file list", M) {
  MAKE_PACKAGE

  Registry reg;
  REQUIRE(reg.getFiles(reg.getEntry(&pkg)).empty());

  reg.push(ver);

  const set<Path> &files = reg.getFiles(reg.getEntry(&pkg));
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
  REQUIRE(entries[0].version.name() == "1.0");
  REQUIRE(entries[0].version.author() == "John Doe");
}

TEST_CASE("forget registry entry", M) {
  MAKE_PACKAGE

  Registry reg;
  reg.forget(reg.push(ver));

  const Registry::Entry &afterForget = reg.getEntry(&pkg);
  REQUIRE(afterForget.id == 0); // uninstalled
}

TEST_CASE("file conflicts", M) {
  Registry reg;

  {
    MAKE_PACKAGE
    reg.push(ver);
  }

  Index ri("Remote Name");
  Category cat("Category Name", &ri);
  Package pkg(Package::ScriptType, "Duplicate Package", &cat);
  Version *ver = new Version("1.0", &pkg);
  Source *src1 = new Source("file", "url", ver);
  Source *src2 = new Source("file2", "url", ver);
  ver->addSource(src1);
  ver->addSource(src2);
  pkg.addVersion(ver);

  CHECK(reg.getEntry(&pkg).id == 0); // uninstalled

  try {
    reg.push(ver);
    FAIL("duplicate is accepted");
  }
  catch(const reapack_error &) {}

  CHECK(reg.getEntry(&pkg).id == 0); // still uninstalled

  vector<Path> conflicts;
  reg.push(ver, &conflicts);

  REQUIRE(conflicts.size() == 1);
  REQUIRE(conflicts[0] == src1->targetPath());

  REQUIRE(reg.getEntry(&pkg).id == 0); // never installed
}

TEST_CASE("get main files", M) {
  MAKE_PACKAGE

  Registry reg;
  REQUIRE((reg.getMainFiles({})).empty());

  Source *main1 = new Source({}, "url", ver);
  main1->setMain(true);
  ver->addSource(main1);

  Source *main2 = new Source({}, "url", ver); // duplicate file
  main2->setMain(true);
  ver->addSource(main2);

  const Registry::Entry &entry = reg.push(ver);

  const vector<string> expected{main1->targetPath().join('/')};
  REQUIRE(reg.getMainFiles(entry) == expected);
}

TEST_CASE("pin registry entry", M) {
  MAKE_PACKAGE

  Registry reg;
  reg.push(ver);

  const Registry::Entry &entry = reg.getEntry(&pkg);
  REQUIRE_FALSE(entry.pinned);

  reg.setPinned(entry, true);
  REQUIRE(reg.getEntry(&pkg).pinned);
  REQUIRE(reg.getEntries(ri.name())[0].pinned);

  reg.setPinned(entry, false);
  REQUIRE_FALSE(reg.getEntry(&pkg).pinned);
}
