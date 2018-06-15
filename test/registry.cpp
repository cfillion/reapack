#include "helper.hpp"

#include <registry.hpp>

#include <errors.hpp>
#include <index.hpp>
#include <package.hpp>
#include <remote.hpp>

using namespace std;

static const char *M = "[registry]";

#define MAKE_PACKAGE \
  RemotePtr re = make_shared<Remote>("Remote Name", "url"); \
  Index ri(re); \
  Category cat("Category Name", &ri); \
  Package pkg(Package::ScriptType, "Hello", &cat); \
  pkg.setDescription("Hello World"); \
  Version ver("1.0", &pkg); \
  ver.setAuthor("John Doe"); \
  Source *src = new Source("file", "url", &ver); \
  ver.addSource(src);

TEST_CASE("query uninstalled package", M) {
  MAKE_PACKAGE

  Registry reg;

  const Registry::Entry &entry = reg.getEntry(&pkg);
  REQUIRE_FALSE(entry);
  REQUIRE(entry.id == 0);
  REQUIRE(entry.version == VersionName());
}

TEST_CASE("query installed package", M) {
  MAKE_PACKAGE

  Registry reg;

  const Registry::Entry &entry = reg.push(&ver);
  REQUIRE(entry);
  REQUIRE(entry.id == 1);
  REQUIRE(entry.remote == "Remote Name");
  REQUIRE(entry.category == "Category Name");
  REQUIRE(entry.package == "Hello");
  REQUIRE(entry.type == Package::ScriptType);
  REQUIRE(entry.version.toString() == "1.0");
  REQUIRE(entry.author == "John Doe");

  const Registry::Entry &selectEntry = reg.getEntry(&pkg);
  REQUIRE(selectEntry.id == entry.id);
  REQUIRE(selectEntry.remote == entry.remote);
  REQUIRE(selectEntry.category == entry.category);
  REQUIRE(selectEntry.package == entry.package);
  REQUIRE(selectEntry.description == entry.description);
  REQUIRE(selectEntry.type == entry.type);
  REQUIRE(selectEntry.version == entry.version);
  REQUIRE(selectEntry.author == entry.author);
}

TEST_CASE("bump version", M) {
  MAKE_PACKAGE

  Version ver2("2.0", &pkg);
  ver2.addSource(new Source("file", "url", &ver2));

  Registry reg;
  reg.push(&ver);

  const Registry::Entry &entry1 = reg.getEntry(&pkg);
  REQUIRE(entry1.version.toString() == "1.0");
  CHECK(entry1.author == "John Doe");

  reg.push(&ver2);
  const Registry::Entry &entry2 = reg.getEntry(&pkg);
  REQUIRE(entry2.version.toString() == "2.0");
  CHECK(entry2.author == "");
  
  REQUIRE(entry2.id == entry1.id);
}

TEST_CASE("get file list", M) {
  MAKE_PACKAGE

  Registry reg;
  REQUIRE(reg.getFiles(reg.getEntry(&pkg)).empty());

  reg.push(&ver);

  const vector<Registry::File> &files = reg.getFiles(reg.getEntry(&pkg));
  REQUIRE(files.size() == 1);
  REQUIRE(files[0].path == src->targetPath());
  REQUIRE(files[0].sections == 0);
  REQUIRE(files[0].type == pkg.type());
}

TEST_CASE("query all packages", M) {
  MAKE_PACKAGE

  const string remote = "Remote Name";

  Registry reg;
  REQUIRE(reg.getEntries(remote).empty());

  reg.push(&ver);

  const vector<Registry::Entry> &entries = reg.getEntries(remote);
  REQUIRE(entries.size() == 1);
  REQUIRE(entries[0].id == 1);
  REQUIRE(entries[0].remote == "Remote Name");
  REQUIRE(entries[0].category == "Category Name");
  REQUIRE(entries[0].package == "Hello");
  REQUIRE(entries[0].type == Package::ScriptType);
  REQUIRE(entries[0].version.toString() == "1.0");
  REQUIRE(entries[0].author == "John Doe");
}

TEST_CASE("forget registry entry", M) {
  MAKE_PACKAGE

  Registry reg;
  reg.forget(reg.push(&ver));

  const Registry::Entry &afterForget = reg.getEntry(&pkg);
  REQUIRE(afterForget.id == 0); // uninstalled
}

TEST_CASE("file conflicts", M) {
  Registry reg;

  {
    MAKE_PACKAGE
    reg.push(&ver);
  }

  RemotePtr re = make_shared<Remote>("Remote Name", "url");
  Index ri(re);
  Category cat("Category Name", &ri);
  Package pkg(Package::ScriptType, "Duplicate Package", &cat);
  Version ver("1.0", &pkg);
  Source *src1 = new Source("file", "url", &ver);
  ver.addSource(src1);
  Source *src2 = new Source("file2", "url", &ver);
  ver.addSource(src2);

  CHECK(reg.getEntry(&pkg).id == 0); // uninstalled

  try {
    reg.push(&ver);
    FAIL("duplicate is accepted");
  }
  catch(const reapack_error &) {}

  CHECK(reg.getEntry(&pkg).id == 0); // still uninstalled

  vector<Path> conflicts;
  const auto &pushResult = reg.push(&ver, &conflicts);
  CHECK(pushResult.id == 0);

  REQUIRE(conflicts.size() == 1);
  REQUIRE(conflicts[0] == src1->targetPath());

  REQUIRE(reg.getEntry(&pkg).id == 0); // never installed
}

TEST_CASE("get main files", M) {
  MAKE_PACKAGE

  Registry reg;
  REQUIRE(reg.getMainFiles({}).empty());

  Source *main1 = new Source({}, "url", &ver);
  main1->setSections(Source::MIDIEditorSection);
  main1->setTypeOverride(Package::EffectType);
  ver.addSource(main1);

  Source *main2 = new Source({}, "url", &ver); // duplicate file ignored
  main2->setSections(Source::MainSection);
  main2->setTypeOverride(Package::EffectType);
  ver.addSource(main2);

  const Registry::Entry &entry = reg.push(&ver);

  const vector<Registry::File> &current = reg.getMainFiles(entry);
  REQUIRE(current.size() == 1);
  REQUIRE(current[0].path == main1->targetPath());
  REQUIRE(current[0].sections == Source::MIDIEditorSection);
  REQUIRE(current[0].type == Package::EffectType);
}

TEST_CASE("pin registry entry", M) {
  MAKE_PACKAGE

  Registry reg;
  reg.push(&ver);

  const Registry::Entry &entry = reg.getEntry(&pkg);
  REQUIRE_FALSE(entry.pinned);

  reg.setPinned(entry, true);
  REQUIRE(reg.getEntry(&pkg).pinned);
  REQUIRE(reg.getEntries(re->name())[0].pinned);

  reg.setPinned(entry, false);
  REQUIRE_FALSE(reg.getEntry(&pkg).pinned);
}

TEST_CASE("get file owner", M) {
  MAKE_PACKAGE

  Registry reg;
  REQUIRE_FALSE(reg.getOwner({}));

  const Registry::Entry &entry = reg.push(&ver);
  REQUIRE(reg.getOwner(src->targetPath()) == entry);
}
