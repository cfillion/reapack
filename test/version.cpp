#include "helper.hpp"

#include <version.hpp>

#include <errors.hpp>
#include <index.hpp>
#include <package.hpp>

#include <sstream>

#define MAKE_PACKAGE \
  Index ri("Index Name"); \
  Category cat("Category Name", &ri); \
  Package pkg(Package::ScriptType, "Package Name", &cat, "remote"); \

static const char *M = "[version]";

TEST_CASE("construct null version", M) {
  const VersionName ver;

  REQUIRE(ver.size() == 0);
  REQUIRE(ver.isStable());
}

TEST_CASE("compare null versions", M) {
  REQUIRE(VersionName("0-beta") > VersionName());
  REQUIRE(VersionName("0") > VersionName());
  REQUIRE(VersionName("0") != VersionName());
  REQUIRE(VersionName() == VersionName());
}

TEST_CASE("parse valid versions", M) {
  VersionName ver;

  SECTION("valid") {
    ver.parse("1.0.1");
    REQUIRE(ver.toString() == "1.0.1");
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
  VersionName ver;
  std::string name;

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
    REQUIRE(std::string{e.what()} == "invalid version name '" + name + "'");
  }

  REQUIRE(ver.toString().empty());
  REQUIRE(ver.size() == 0);
}

TEST_CASE("parse version failsafe", M) {
  VersionName ver;

  SECTION("valid") {
    REQUIRE(ver.tryParse("1.0"));

    REQUIRE(ver.toString() == "1.0");
    REQUIRE(ver.size() == 2);
  }

  SECTION("invalid") {
    REQUIRE_FALSE(ver.tryParse("hello"));

    std::string error;
    REQUIRE_FALSE(ver.tryParse("world", &error));

    REQUIRE(ver.toString().empty());
    REQUIRE(ver.size() == 0);
    REQUIRE(error == "invalid version name 'world'");
  }
}

TEST_CASE("decimal version", M) {
  VersionName ver("5.05");
  REQUIRE(ver == VersionName("5.5"));
  REQUIRE(ver < VersionName("5.50"));
}

TEST_CASE("5 version segments", M) {
  REQUIRE(VersionName("1.1.1.1.0") < VersionName("1.1.1.1.1"));
  REQUIRE(VersionName("1.1.1.1.1") == VersionName("1.1.1.1.1"));
  REQUIRE(VersionName("1.1.1.1.1") < VersionName("1.1.1.1.2"));
  REQUIRE(VersionName("1.1.1.1.1") < VersionName("1.1.1.2.0"));
}

TEST_CASE("version segment overflow", M) {
  try {
    VersionName ver("9999999999999999999999");
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(std::string{e.what()} == "version segment overflow in '9999999999999999999999'");
  }
}

TEST_CASE("compare versions", M) {
  SECTION("equality") {
    REQUIRE(VersionName("1.0").compare({"1.0"}) == 0);

    REQUIRE(VersionName("1.0") == VersionName("1.0"));
    REQUIRE_FALSE(VersionName("1.0") == VersionName("1.1"));
  }

  SECTION("inequality") {
    REQUIRE_FALSE(VersionName("1.0") != VersionName("1.0"));
    REQUIRE(VersionName("1.0") != VersionName("1.1"));
  }

  SECTION("less than") {
    REQUIRE(VersionName("1.0").compare({"1.1"}) == -1);

    REQUIRE(VersionName("1.0") < VersionName("1.1"));
    REQUIRE_FALSE(VersionName("1.0") < VersionName("1.0"));
    REQUIRE_FALSE(VersionName("1.1") < VersionName("1.0"));
  }

  SECTION("less than or equal") {
    REQUIRE(VersionName("1.0") <= VersionName("1.1"));
    REQUIRE(VersionName("1.0") <= VersionName("1.0"));
    REQUIRE_FALSE(VersionName("1.1") <= VersionName("1.0"));
  }

  SECTION("greater than") {
    REQUIRE(VersionName("1.1").compare({"1.0"}) == 1);

    REQUIRE_FALSE(VersionName("1.0") > VersionName("1.1"));
    REQUIRE_FALSE(VersionName("1.0") > VersionName("1.0"));
    REQUIRE(VersionName("1.1") > VersionName("1.0"));
  }

  SECTION("greater than or equal") {
    REQUIRE_FALSE(VersionName("1.0") >= VersionName("1.1"));
    REQUIRE(VersionName("1.0") >= VersionName("1.0"));
    REQUIRE(VersionName("1.1") >= VersionName("1.0"));
  }
}

TEST_CASE("compare versions with more or less segments", M) {
  REQUIRE(VersionName("1") == VersionName("1.0.0.0"));
  REQUIRE(VersionName("1") != VersionName("1.0.0.1"));

  REQUIRE(VersionName("1.0.0.0") == VersionName("1"));
  REQUIRE(VersionName("1.0.0.1") != VersionName("1"));
}

TEST_CASE("prerelease versions", M) {
  SECTION("detect") {
    REQUIRE(VersionName("1.0").isStable());
    REQUIRE_FALSE(VersionName("1.0b").isStable());
    REQUIRE_FALSE(VersionName("1.0-beta").isStable());
    REQUIRE_FALSE(VersionName("1.0-beta1").isStable());
  }

  SECTION("compare") {
    REQUIRE(VersionName("0.9") < VersionName("1.0a"));
    REQUIRE(VersionName("1.0a.2") < VersionName("1.0b.1"));
    REQUIRE(VersionName("1.0-beta1") < VersionName("1.0"));

    REQUIRE(VersionName("1.0b") < VersionName("1.0.1"));
    REQUIRE(VersionName("1.0.1") > VersionName("1.0b"));
  }
}

TEST_CASE("copy version constructor", M) {
  const VersionName original("1.1test");
  const VersionName copy(original);

  REQUIRE(copy.toString() == "1.1test");
  REQUIRE(copy.size() == original.size());
  REQUIRE(copy.isStable() == original.isStable());
}

TEST_CASE("version full name", M) {
  MAKE_PACKAGE;

  Version ver("1.0", &pkg);
  REQUIRE(ver.fullName() == "Index Name/Category Name/Package Name v1.0");
}

TEST_CASE("add source", M) {
  MAKE_PACKAGE;

  Version ver("1.0", &pkg);
  CHECK(ver.sources().size() == 0);

  Source *src = new Source("a", "b", &ver);
  REQUIRE(ver.addSource(src));

  CHECK(ver.sources().size() == 1);
  REQUIRE(ver.source(0) == src);
}

TEST_CASE("add owned source", M) {
  Version ver1("1", nullptr);
  Version ver2("1", nullptr);
  Source *src = new Source("a", "b", &ver2);

  try {
    ver1.addSource(src);
    FAIL();
  }
  catch(const reapack_error &e) {
    delete src;
    REQUIRE(std::string{e.what()} == "source belongs to another version");
  }
}

TEST_CASE("duplicate sources", M) {
  MAKE_PACKAGE;
  Version ver("1.0", &pkg);

  Source *src = new Source({}, "b", &ver);
  CHECK(ver.addSource(src) == true);
  REQUIRE(ver.addSource(src) == false);

  REQUIRE(ver.sources().size() == 1);
  REQUIRE(ver.source(0) == src);
}

TEST_CASE("list files", M) {
  MAKE_PACKAGE;
  Version ver("1.0", &pkg);

  Source *src1 = new Source("file", "url", &ver);
  ver.addSource(src1);

  Path path1;
  path1.append("Scripts");
  path1.append("Index Name");
  path1.append("Category Name");
  path1.append("file");

  const std::set<Path> expected{path1};
  REQUIRE(ver.files() == expected);
}

TEST_CASE("drop sources for unknown platforms", M) {
  MAKE_PACKAGE;
  Version ver("1.0", &pkg);
  Source src("a", "b", &ver);
  src.setPlatform(Platform::Unknown);
  REQUIRE_FALSE(ver.addSource(&src));
  REQUIRE(ver.sources().size() == 0);
}

TEST_CASE("version author", M) {
  Version ver("1.0", nullptr);
  CHECK(ver.author().empty());
  REQUIRE(ver.displayAuthor() == "Unknown");

  ver.setAuthor("cfillion");
  REQUIRE(ver.author() == "cfillion");
  REQUIRE(ver.displayAuthor() == ver.author());

  REQUIRE(Version::displayAuthor({}) == "Unknown");
  REQUIRE(Version::displayAuthor("cfillion") == "cfillion");
}

TEST_CASE("version date", M) {
  Version ver("1.0", nullptr);

  ver.setTime("2016-02-12T01:16:40Z");
  REQUIRE(ver.time().year() == 2016);

  ver.setTime("hello world");
  REQUIRE(ver.time().year() == 2016);
}

TEST_CASE("output version", M) {
  std::ostringstream stream;
  Version ver("1.2.3", nullptr);

  SECTION("empty version") {
    stream << ver;
    REQUIRE(stream.str() == "v1.2.3\r\n  No changelog");
  }

  SECTION("with author") {
    ver.setAuthor("Hello World");
    stream << ver;
    REQUIRE(stream.str() == "v1.2.3 by Hello World\r\n  No changelog");
  }

  SECTION("with time") {
    ver.setTime("2016-01-02T00:42:11Z");
    stream << ver;
    REQUIRE(stream.str() == "v1.2.3 – January 02 2016\r\n  No changelog");
  }

  SECTION("with changelog") {
    ver.setChangelog("+ added super cool feature\n+ fixed all the bugs!");
    stream << ver;
    REQUIRE(stream.str() == "v1.2.3\r\n  + added super cool feature\r\n  + fixed all the bugs!");
  }

  SECTION("changelog with empty lines") {
    ver.setChangelog("line1\n\nline2");
    stream << ver; // no crash!
    REQUIRE(stream.str() == "v1.2.3\r\n  line1\r\n\r\n  line2");
  }
}
