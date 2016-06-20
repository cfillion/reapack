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
  REQUIRE(ver.displayTime().empty());
  REQUIRE(ver.package() == nullptr);
  REQUIRE(ver.mainSources().empty());
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

  try {
    SECTION("only letters") {
      ver.parse("hello");
      FAIL();
    }

    SECTION("leading letter") {
      ver.parse("v1.0");
      FAIL();
    }

    SECTION("empty") {
      ver.parse("");
      FAIL();
    }

  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "invalid version name");
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
    REQUIRE(string(e.what()) == "version segment overflow");
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
  SECTION("no package") {
    Version ver("1.0");
    REQUIRE(ver.fullName() == "v1.0");
  }

  SECTION("with package") {
    Package pkg(Package::UnknownType, "file.name");
    Version ver("1.0", &pkg);

    REQUIRE(ver.fullName() == "file.name v1.0");
  }

  SECTION("with category") {
    Category cat("Category Name");
    Package pkg(Package::UnknownType, "file.name", &cat);
    Version ver("1.0", &pkg);

    REQUIRE(ver.fullName() == "Category Name/file.name v1.0");
  }

  SECTION("with index") {
    Index ri("Remote Name");
    Category cat("Category Name", &ri);
    Package pkg(Package::UnknownType, "file.name", &cat);
    Version ver("1.0", &pkg);

    REQUIRE(ver.fullName() == "Remote Name/Category Name/file.name v1.0");
  }
}

TEST_CASE("add source", M) {
  MAKE_VERSION

  CHECK(ver.sources().size() == 0);

  Source *src = new Source("a", "b", &ver);
  ver.addSource(src);

  CHECK(ver.sources().size() == 1);
  CHECK(ver.mainSources().empty());

  REQUIRE(src->version() == &ver);
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

TEST_CASE("add main sources", M) {
  MAKE_VERSION

  Source *src1 = new Source({}, "b", &ver);
  src1->setMain(true);
  ver.addSource(src1);

  Source *src2 = new Source({}, "b", &ver);
  src2->setMain(true);
  ver.addSource(src2);

  REQUIRE(ver.mainSources().size() == 2);
  REQUIRE(ver.mainSources()[0] == src1);
  REQUIRE(ver.mainSources()[1] == src2);
}

TEST_CASE("duplicate sources", M) {
  MAKE_VERSION

  Source *src = new Source({}, "b", &ver);
  ver.addSource(src);
  ver.addSource(new Source({}, "b", &ver));

  REQUIRE(ver.sources().size() == 2);
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
  Source *src = new Source("a", "b", &ver);
  src->setPlatform(Platform::UnknownPlatform);
  ver.addSource(src);

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
  CHECK(ver.time().tm_year == 0);
  CHECK(ver.time().tm_mon == 0);
  CHECK(ver.time().tm_mday == 0);
  CHECK(ver.displayTime() == "");

  SECTION("valid") {
    ver.setTime("2016-02-12T01:16:40Z");
    REQUIRE(ver.time().tm_year == 2016 - 1900);
    REQUIRE(ver.time().tm_mon == 2 - 1);
    REQUIRE(ver.time().tm_mday == 12);
    REQUIRE(ver.displayTime() != "");
  }

  SECTION("garbage") {
    ver.setTime("hello world");
    REQUIRE(ver.time().tm_year == 0);
    REQUIRE(ver.displayTime() == "");
  }

  SECTION("out of range") {
    ver.setTime("2016-99-99T99:99:99Z");
    REQUIRE(ver.displayTime() == "");
  }
}

TEST_CASE("copy version constructor", M) {
  const Package pkg(Package::UnknownType, "Hello");

  Version original("1.1test", &pkg);
  original.setAuthor("John Doe");
  original.setChangelog("Initial release");
  original.setTime("2016-02-12T01:16:40Z");

  const Version copy1(original);
  REQUIRE(copy1.name() == "1.1test");
  REQUIRE(copy1.size() == original.size());
  REQUIRE(copy1.isStable() == original.isStable());
  REQUIRE(copy1.author() == original.author());
  REQUIRE(copy1.changelog() == original.changelog());
  REQUIRE(copy1.displayTime() == original.displayTime());
  REQUIRE(copy1.package() == nullptr);
  REQUIRE(copy1.mainSources().empty());
  REQUIRE(copy1.sources().empty());

  const Version copy2(original, &pkg);
  REQUIRE(copy2.package() == &pkg);
}
