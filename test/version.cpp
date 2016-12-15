#include <catch.hpp>

#include "helper/io.hpp"

#include <version.hpp>

#include <errors.hpp>
#include <index.hpp>
#include <package.hpp>

using namespace std;

#define MAKE_VERSION \
  Index ri("Remote Name"); \
  Category cat("Category Name", &ri); \
  Package pkg(Package::ScriptType, "Hello", &cat); \
  Version ver("1", &pkg);

static const char *M = "[version]";

TEST_CASE("construct null version", M) {
  const Version ver;

  REQUIRE(ver.size() == 0);
  REQUIRE(ver.isStable());
  REQUIRE_FALSE(ver.time().isValid());
  REQUIRE(ver.package() == nullptr);
}

TEST_CASE("compare null versions", M) {
  REQUIRE(Version("0-beta") > Version());
  REQUIRE(Version("0") > Version());
  REQUIRE(Version("0") != Version());
  REQUIRE(Version() == Version());
}

TEST_CASE("parse valid versions", M) {
  Version ver;

  SECTION("valid") {
    ver.parse("1.0.1");
    REQUIRE(ver.name() == "1.0.1");
    REQUIRE(ver.size() == 3);
  }

  SECTION("prerelease set/unset") {
    ver.parse("1.0beta");
    REQUIRE_FALSE(ver.isStable());
    ver.parse("1.0");
    REQUIRE(ver.isStable());
  }

  SECTION("equal to a null version") {
    ver.parse("0");
  }
}

TEST_CASE("parse invalid versions", M) {
  Version ver;
  string name;

  SECTION("only letters")
    name = "hello";

  SECTION("leading letter")
    name = "v1.0";

  SECTION("empty")
    name = {};

  try {
    ver.parse(name);
    FAIL(string("'") + name + "' was accepted");
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == string("invalid version name '") + name + "'");
  }

  REQUIRE(ver.name().empty());
  REQUIRE(ver.size() == 0);
}

TEST_CASE("parse version failsafe", M) {
  Version ver;

  SECTION("valid") {
    REQUIRE(ver.tryParse("1.0"));

    REQUIRE(ver.name() == "1.0");
    REQUIRE(ver.size() == 2);
  }

  SECTION("invalid") {
    REQUIRE_FALSE(ver.tryParse("hello"));

    REQUIRE(ver.name().empty());
    REQUIRE(ver.size() == 0);
  }
}

TEST_CASE("decimal version", M) {
  Version ver("5.05");
  REQUIRE(ver == Version("5.5"));
  REQUIRE(ver < Version("5.50"));
}

TEST_CASE("5 version segments", M) {
  REQUIRE(Version("1.1.1.1.0") < Version("1.1.1.1.1"));
  REQUIRE(Version("1.1.1.1.1") == Version("1.1.1.1.1"));
  REQUIRE(Version("1.1.1.1.1") < Version("1.1.1.1.2"));
  REQUIRE(Version("1.1.1.1.1") < Version("1.1.1.2.0"));
}

TEST_CASE("version segment overflow", M) {
  try {
    Version ver("9999999999999999999999");
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "version segment overflow in '9999999999999999999999'");
  }
}

TEST_CASE("compare versions", M) {
  SECTION("equality") {
    REQUIRE(Version("1.0").compare(Version("1.0")) == 0);

    REQUIRE(Version("1.0") == Version("1.0"));
    REQUIRE_FALSE(Version("1.0") == Version("1.1"));
  }

  SECTION("inequality") {
    REQUIRE_FALSE(Version("1.0") != Version("1.0"));
    REQUIRE(Version("1.0") != Version("1.1"));
  }

  SECTION("less than") {
    REQUIRE(Version("1.0").compare(Version("1.1")) == -1);

    REQUIRE(Version("1.0") < Version("1.1"));
    REQUIRE_FALSE(Version("1.0") < Version("1.0"));
    REQUIRE_FALSE(Version("1.1") < Version("1.0"));
  }

  SECTION("less than or equal") {
    REQUIRE(Version("1.0") <= Version("1.1"));
    REQUIRE(Version("1.0") <= Version("1.0"));
    REQUIRE_FALSE(Version("1.1") <= Version("1.0"));
  }

  SECTION("greater than") {
    REQUIRE(Version("1.1").compare(Version("1.0")) == 1);

    REQUIRE_FALSE(Version("1.0") > Version("1.1"));
    REQUIRE_FALSE(Version("1.0") > Version("1.0"));
    REQUIRE(Version("1.1") > Version("1.0"));
  }

  SECTION("greater than or equal") {
    REQUIRE_FALSE(Version("1.0") >= Version("1.1"));
    REQUIRE(Version("1.0") >= Version("1.0"));
    REQUIRE(Version("1.1") >= Version("1.0"));
  }
}

TEST_CASE("compare versions with more or less segments", M) {
  REQUIRE(Version("1") == Version("1.0.0.0"));
  REQUIRE(Version("1") != Version("1.0.0.1"));

  REQUIRE(Version("1.0.0.0") == Version("1"));
  REQUIRE(Version("1.0.0.1") != Version("1"));
}

TEST_CASE("prerelease versions", M) {
  SECTION("detect") {
    REQUIRE(Version("1.0").isStable());
    REQUIRE_FALSE(Version("1.0b").isStable());
    REQUIRE_FALSE(Version("1.0-beta").isStable());
    REQUIRE_FALSE(Version("1.0-beta1").isStable());
  }

  SECTION("compare") {
    REQUIRE(Version("0.9") < Version("1.0a"));
    REQUIRE(Version("1.0a.2") < Version("1.0b.1"));
    REQUIRE(Version("1.0-beta1") < Version("1.0"));

    REQUIRE(Version("1.0b") < Version("1.0.1"));
    REQUIRE(Version("1.0.1") > Version("1.0b"));
  }
}

TEST_CASE("version full name", M) {
  Index ri("Index Name");
  Category cat("Category Name", &ri);
  Package pkg(Package::UnknownType, "file.name", &cat);
  Version ver("1.0", &pkg);

  REQUIRE(ver.fullName() == "Index Name/Category Name/file.name v1.0");
}

TEST_CASE("add source", M) {
  MAKE_VERSION

  CHECK(ver.sources().size() == 0);

  Source *src = new Source("a", "b", &ver);
  REQUIRE(ver.addSource(src));

  CHECK(ver.sources().size() == 1);
  REQUIRE(ver.source(0) == src);
}

TEST_CASE("add owned source", M) {
  MAKE_VERSION

  Version ver2("1");
  Source *src = new Source("a", "b", &ver2);

  try {
    ver.addSource(src);
    FAIL();
  }
  catch(const reapack_error &e) {
    delete src;
    REQUIRE(string(e.what()) == "source belongs to another version");
  }
}

TEST_CASE("duplicate sources", M) {
  MAKE_VERSION

  Source *src = new Source({}, "b", &ver);
  CHECK(ver.addSource(src) == true);
  REQUIRE(ver.addSource(src) == false);

  REQUIRE(ver.sources().size() == 1);
  REQUIRE(ver.source(0) == src);
}

TEST_CASE("list files", M) {
  MAKE_VERSION

  Source *src1 = new Source("file", "url", &ver);
  ver.addSource(src1);

  Path path1;
  path1.append("Scripts");
  path1.append("Remote Name");
  path1.append("Category Name");
  path1.append("file");

  const set<Path> expected{path1};
  REQUIRE(ver.files() == expected);
}

TEST_CASE("drop sources for unknown platforms", M) {
  MAKE_VERSION
  Source src("a", "b", &ver);
  src.setPlatform(Platform::UnknownPlatform);
  REQUIRE_FALSE(ver.addSource(&src));
  REQUIRE(ver.sources().size() == 0);
}

TEST_CASE("version author", M) {
  Version ver("1.0");
  CHECK(ver.author().empty());

  ver.setAuthor("cfillion");
  REQUIRE(ver.author() == "cfillion");
}

TEST_CASE("version date", M) {
  Version ver("1.0");

  ver.setTime("2016-02-12T01:16:40Z");
  REQUIRE(ver.time().year() == 2016);

  ver.setTime("hello world");
  REQUIRE(ver.time().year() == 2016);
}

TEST_CASE("copy version constructor", M) {
  Index ri("Remote Name");
  Category cat("Category Name", &ri);
  Package pkg(Package::ScriptType, "Hello", &cat);

  Version original("1.1test", &pkg);
  original.setAuthor("John Doe");
  original.setChangelog("Initial release");
  original.setTime("2016-02-12T01:16:40Z");

  const Version copy(original);
  REQUIRE(copy.name() == "1.1test");
  REQUIRE(copy.size() == original.size());
  REQUIRE(copy.isStable() == original.isStable());
  REQUIRE(copy.author() == original.author());
  REQUIRE(copy.changelog().empty());
  REQUIRE_FALSE(copy.time().isValid());
  REQUIRE(copy.package() == nullptr);
  REQUIRE(copy.sources().empty());
}
