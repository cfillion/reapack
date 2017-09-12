#include "helper.hpp"

#include <receipt.hpp>

#include <index.hpp>

using namespace std;

static constexpr const char *M = "[receipt]";

#define MAKE_VERSION \
  IndexPtr ri = make_shared<Index>("Index Name"); \
  Category cat("Category Name", ri.get()); \
  Package pkg(Package::ScriptType, "Package Name", &cat); \
  Version ver("1.0", &pkg);

TEST_CASE("set isRestartNeeded", M) {
  Receipt r;
  REQUIRE_FALSE(r.isRestartNeeded());
  r.setRestartNeeded(true);
  REQUIRE(r.isRestartNeeded());
}

TEST_CASE("non-empty receipt", M) {
  MAKE_VERSION;

  Receipt r;
  REQUIRE(r.empty());

  SECTION("install")
    r.addInstall({&ver, {}});

  SECTION("removal")
    r.addRemoval(Path("hello/world"));

  SECTION("export")
    r.addExport(Path("hello/world"));

  SECTION("error")
    r.addError({"message", "context"});

  REQUIRE_FALSE(r.empty());
}

TEST_CASE("install scratch or downgrade", M) {
  MAKE_VERSION;
  Receipt r;

  REQUIRE(r.installs().empty());

  SECTION("install from stratch")
    r.addInstall({&ver, {}});

  SECTION("downgrade") {
    Registry::Entry entry{1};
    entry.version = VersionName("2.0");
    r.addInstall({&ver, entry});
  }

  REQUIRE(r.installs().size() == 1);
  REQUIRE(r.updates().empty());
}

TEST_CASE("detect update tickets", M) {
  MAKE_VERSION;

  Receipt r;
  REQUIRE(r.updates().empty());

  Registry::Entry entry{1};
  entry.version = VersionName("0.9");
  r.addInstall({&ver, entry});

  REQUIRE(r.updates().size() == 1);
  REQUIRE(r.installs().empty());
}
