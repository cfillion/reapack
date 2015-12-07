#include <catch.hpp>

#include <database.hpp>
#include <package.hpp>
#include <registry.hpp>

using namespace std;

static const char *M = "[registry]";

#define MAKE_PACKAGE \
  Database db; \
  db.setName("Hello"); \
  Category cat("Hello"); \
  cat.setDatabase(&db); \
  Package pkg(Package::ScriptType, "Hello"); \
  pkg.setCategory(&cat); \
  Version *ver = new Version("1.0"); \
  Source *src = new Source(Source::GenericPlatform, "https://..."); \
  ver->addSource(src); \
  pkg.addVersion(ver);

TEST_CASE("query uninstalled package", M) {
  MAKE_PACKAGE

  Registry reg;
  REQUIRE(reg.query(&pkg) == Registry::Uninstalled);
}

TEST_CASE("query up to date pacakge", M) {
  MAKE_PACKAGE

  Registry reg;
  reg.push(&pkg);
  REQUIRE(reg.query(&pkg) == Registry::UpToDate);
}

TEST_CASE("bump version", M) {
  MAKE_PACKAGE

  Version *ver2 = new Version("2.0");
  ver2->addSource(new Source(Source::GenericPlatform, "https://..."));

  Registry reg;
  reg.push(&pkg);
  pkg.addVersion(ver2);
  REQUIRE(reg.query(&pkg) == Registry::UpdateAvailable);
  reg.push(&pkg);
  REQUIRE(reg.query(&pkg) == Registry::UpToDate);
}
