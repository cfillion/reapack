#include "helper.hpp"

#include <version.hpp>

#include <errors.hpp>
#include <index.hpp>
#include <package.hpp>

using namespace std;

#define MAKE_PACKAGE \
  Index ri(L("Index Name")); \
  Category cat(L("Category Name"), &ri); \
  Package pkg(Package::ScriptType, L("Package Name"), &cat); \

static const char *M = "[version]";

TEST_CASE("construct null version", M) {
  const VersionName ver;

  REQUIRE(ver.size() == 0);
  REQUIRE(ver.isStable());
}

TEST_CASE("compare null versions", M) {
  REQUIRE(VersionName(L("0-beta")) > VersionName());
  REQUIRE(VersionName(L("0")) > VersionName());
  REQUIRE(VersionName(L("0")) != VersionName());
  REQUIRE(VersionName() == VersionName());
}

TEST_CASE("parse valid versions", M) {
  VersionName ver;

  SECTION("valid") {
    ver.parse(L("1.0.1"));
    REQUIRE(ver.toString() == L("1.0.1"));
    REQUIRE(ver.size() == 3);
  }

  SECTION("prerelease set/unset") {
    ver.parse(L("1.0beta"));
    REQUIRE_FALSE(ver.isStable());
    ver.parse(L("1.0"));
    REQUIRE(ver.isStable());
  }

  SECTION("equal to a null version") {
    ver.parse(L("0"));
  }
}

TEST_CASE("parse invalid versions", M) {
  VersionName ver;
  string name;

  SECTION("only letters")
    name = "hello";

  SECTION("leading letter")
    name = "v1.0";

  SECTION("empty")
    name = {};

  try {
    ver.parse(name);
    FAIL("'" + name + "' was accepted");
  }
  catch(const reapack_error &e) {
    REQUIRE((string)e.what() == "invalid version name '" + name + "'");
  }

  REQUIRE(ver.toString().empty());
  REQUIRE(ver.size() == 0);
}

TEST_CASE("parse version failsafe", M) {
  VersionName ver;

  SECTION("valid") {
    REQUIRE(ver.tryParse(L("1.0")));

    REQUIRE(ver.toString() == L("1.0"));
    REQUIRE(ver.size() == 2);
  }

  SECTION("invalid") {
    REQUIRE_FALSE(ver.tryParse(L("hello")));

    String error;
    REQUIRE_FALSE(ver.tryParse(L("world"), &error));

    REQUIRE(ver.toString().empty());
    REQUIRE(ver.size() == 0);
    REQUIRE(error == L("invalid version name 'world'"));
  }
}

TEST_CASE("decimal version", M) {
  VersionName ver(L("5.05"));
  REQUIRE(ver == VersionName(L("5.5")));
  REQUIRE(ver < VersionName(L("5.50")));
}

TEST_CASE("5 version segments", M) {
  REQUIRE(VersionName(L("1.1.1.1.0")) < VersionName(L("1.1.1.1.1")));
  REQUIRE(VersionName(L("1.1.1.1.1")) == VersionName(L("1.1.1.1.1")));
  REQUIRE(VersionName(L("1.1.1.1.1")) < VersionName(L("1.1.1.1.2")));
  REQUIRE(VersionName(L("1.1.1.1.1")) < VersionName(L("1.1.1.2.0")));
}

TEST_CASE("version segment overflow", M) {
  try {
    VersionName ver(L("9999999999999999999999"));
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(e.what() == L("version segment overflow in '9999999999999999999999'"));
  }
}

TEST_CASE("compare versions", M) {
  SECTION("equality") {
    REQUIRE(VersionName(L("1.0")).compare({L("1.0")}) == 0);

    REQUIRE(VersionName(L("1.0")) == VersionName(L("1.0")));
    REQUIRE_FALSE(VersionName(L("1.0")) == VersionName(L("1.1")));
  }

  SECTION("inequality") {
    REQUIRE_FALSE(VersionName(L("1.0")) != VersionName(L("1.0")));
    REQUIRE(VersionName(L("1.0")) != VersionName(L("1.1")));
  }

  SECTION("less than") {
    REQUIRE(VersionName(L("1.0")).compare({L("1.1")}) == -1);

    REQUIRE(VersionName(L("1.0")) < VersionName(L("1.1")));
    REQUIRE_FALSE(VersionName(L("1.0")) < VersionName(L("1.0")));
    REQUIRE_FALSE(VersionName(L("1.1")) < VersionName(L("1.0")));
  }

  SECTION("less than or equal") {
    REQUIRE(VersionName(L("1.0")) <= VersionName(L("1.1")));
    REQUIRE(VersionName(L("1.0")) <= VersionName(L("1.0")));
    REQUIRE_FALSE(VersionName(L("1.1")) <= VersionName(L("1.0")));
  }

  SECTION("greater than") {
    REQUIRE(VersionName(L("1.1")).compare({L("1.0")}) == 1);

    REQUIRE_FALSE(VersionName(L("1.0")) > VersionName(L("1.1")));
    REQUIRE_FALSE(VersionName(L("1.0")) > VersionName(L("1.0")));
    REQUIRE(VersionName(L("1.1")) > VersionName(L("1.0")));
  }

  SECTION("greater than or equal") {
    REQUIRE_FALSE(VersionName(L("1.0")) >= VersionName(L("1.1")));
    REQUIRE(VersionName(L("1.0")) >= VersionName(L("1.0")));
    REQUIRE(VersionName(L("1.1")) >= VersionName(L("1.0")));
  }
}

TEST_CASE("compare versions with more or less segments", M) {
  REQUIRE(VersionName(L("1")) == VersionName(L("1.0.0.0")));
  REQUIRE(VersionName(L("1")) != VersionName(L("1.0.0.1")));

  REQUIRE(VersionName(L("1.0.0.0")) == VersionName(L("1")));
  REQUIRE(VersionName(L("1.0.0.1")) != VersionName(L("1")));
}

TEST_CASE("prerelease versions", M) {
  SECTION("detect") {
    REQUIRE(VersionName(L("1.0")).isStable());
    REQUIRE_FALSE(VersionName(L("1.0b")).isStable());
    REQUIRE_FALSE(VersionName(L("1.0-beta")).isStable());
    REQUIRE_FALSE(VersionName(L("1.0-beta1")).isStable());
  }

  SECTION("compare") {
    REQUIRE(VersionName(L("0.9")) < VersionName(L("1.0a")));
    REQUIRE(VersionName(L("1.0a.2")) < VersionName(L("1.0b.1")));
    REQUIRE(VersionName(L("1.0-beta1")) < VersionName(L("1.0")));

    REQUIRE(VersionName(L("1.0b")) < VersionName(L("1.0.1")));
    REQUIRE(VersionName(L("1.0.1")) > VersionName(L("1.0b")));
  }
}

TEST_CASE("copy version constructor", M) {
  const VersionName original(L("1.1test"));
  const VersionName copy(original);

  REQUIRE(copy.toString() == L("1.1test"));
  REQUIRE(copy.size() == original.size());
  REQUIRE(copy.isStable() == original.isStable());
}

TEST_CASE("version full name", M) {
  MAKE_PACKAGE;

  Version ver(L("1.0"), &pkg);
  REQUIRE(ver.fullName() == L("Index Name/Category Name/Package Name v1.0"));
}

TEST_CASE("add source", M) {
  MAKE_PACKAGE;

  Version ver(L("1.0"), &pkg);
  CHECK(ver.sources().size() == 0);

  Source *src = new Source(L("a"), L("b"), &ver);
  REQUIRE(ver.addSource(src));

  CHECK(ver.sources().size() == 1);
  REQUIRE(ver.source(0) == src);
}

TEST_CASE("add owned source", M) {
  Version ver1(L("1"), nullptr);
  Version ver2(L("1"), nullptr);
  Source *src = new Source(L("a"), L("b"), &ver2);

  try {
    ver1.addSource(src);
    FAIL();
  }
  catch(const reapack_error &e) {
    delete src;
    REQUIRE(e.what() == L("source belongs to another version"));
  }
}

TEST_CASE("duplicate sources", M) {
  MAKE_PACKAGE;
  Version ver(L("1.0"), &pkg);

  Source *src = new Source({}, L("b"), &ver);
  CHECK(ver.addSource(src) == true);
  REQUIRE(ver.addSource(src) == false);

  REQUIRE(ver.sources().size() == 1);
  REQUIRE(ver.source(0) == src);
}

TEST_CASE("list files", M) {
  MAKE_PACKAGE;
  Version ver(L("1.0"), &pkg);

  Source *src1 = new Source(L("file"), L("url"), &ver);
  ver.addSource(src1);

  Path path1;
  path1.append(L("Scripts"));
  path1.append(L("Index Name"));
  path1.append(L("Category Name"));
  path1.append(L("file"));

  const set<Path> expected{path1};
  REQUIRE(ver.files() == expected);
}

TEST_CASE("drop sources for unknown platforms", M) {
  MAKE_PACKAGE;
  Version ver(L("1.0"), &pkg);
  Source src(L("a"), L("b"), &ver);
  src.setPlatform(Platform::UnknownPlatform);
  REQUIRE_FALSE(ver.addSource(&src));
  REQUIRE(ver.sources().size() == 0);
}

TEST_CASE("version author", M) {
  Version ver(L("1.0"), nullptr);
  CHECK(ver.author().empty());
  REQUIRE(ver.displayAuthor() == L("Unknown"));

  ver.setAuthor(L("cfillion"));
  REQUIRE(ver.author() == L("cfillion"));
  REQUIRE(ver.displayAuthor() == ver.author());

  REQUIRE(Version::displayAuthor({}) == L("Unknown"));
  REQUIRE(Version::displayAuthor(L("cfillion")) == L("cfillion"));
}

TEST_CASE("version date", M) {
  Version ver(L("1.0"), nullptr);

  ver.setTime("2016-02-12T01:16:40Z");
  REQUIRE(ver.time().year() == 2016);

  ver.setTime("hello world");
  REQUIRE(ver.time().year() == 2016);
}
