#include <catch.hpp>

#include <index.hpp>
#include <package.hpp>
#include <registry.hpp>

using namespace std;

static const char *M = "[registry]";

#define MAKE_PACKAGE \
  RemoteIndex ri("Hello"); \
  Category cat("Hello", &ri); \
  Package pkg(Package::ScriptType, "Hello", &cat); \
  Version *ver = new Version("1.0", &pkg); \
  Source *src = new Source(Source::GenericPlatform, "file", "url", ver); \
  ver->addSource(src); \
  pkg.addVersion(ver);

TEST_CASE("query uninstalled package", M) {
  MAKE_PACKAGE

  Registry reg;

  const Registry::QueryResult res = reg.query(&pkg);
  REQUIRE(res.status == Registry::Uninstalled);
  REQUIRE(res.version == 0);
}

TEST_CASE("query up to date pacakge", M) {
  MAKE_PACKAGE

  Registry reg;
  reg.push(ver);

  const Registry::QueryResult res = reg.query(&pkg);
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

  const Registry::QueryResult res1 = reg.query(&pkg);
  REQUIRE(res1.status == Registry::UpdateAvailable);
  REQUIRE(res1.version == Version("1.0").code());

  reg.push(ver2);
  const Registry::QueryResult res2 = reg.query(&pkg);
  REQUIRE(res2.status == Registry::UpToDate);
  REQUIRE(res2.version == Version("2.0").code());
}
