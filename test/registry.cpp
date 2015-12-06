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

TEST_CASE("version of uninstalled", M) {
  MAKE_PACKAGE

  Registry reg;
  REQUIRE(reg.versionOf(&pkg) == string());
}

TEST_CASE("version of installed", M) {
  MAKE_PACKAGE

  Registry reg;
  reg.push(&pkg);
  REQUIRE(reg.versionOf(&pkg) == "1.0");
}

TEST_CASE("bump version", M) {
  MAKE_PACKAGE

  Version *ver2 = new Version("2.0");
  ver2->addSource(new Source(Source::GenericPlatform, "https://..."));

  Registry reg;
  reg.push(&pkg);
  pkg.addVersion(ver2);
  reg.push(&pkg);
  REQUIRE(reg.versionOf(&pkg) == "2.0");
}
