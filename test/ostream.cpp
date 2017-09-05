#include "helper.hpp"

#include <ostream.hpp>

#include <string.hpp>
#include <version.hpp>

static const char *M = "[ostream]";

TEST_CASE("test number formatting", M) {
  OutputStream stream;
  stream << 1234;
  REQUIRE(stream.str() == L("1,234"));
}

TEST_CASE("test indent string", M) {
  OutputStream stream;

  SECTION("simple")
    stream.indented(L("line1\nline2"));

  SECTION("already indented")
    stream.indented(L("  line1\n  line2"));

  REQUIRE(stream.str() == L("  line1\r\n  line2\r\n"));
}

TEST_CASE("output version", M) {
  OutputStream stream;
  Version ver(L("1.2.3"), nullptr);

  SECTION("empty version") {
    stream << ver;
    REQUIRE(stream.str() == L("v1.2.3\r\n  No changelog\r\n"));
  }

  SECTION("with author") {
    ver.setAuthor(L("Hello World"));
    stream << ver;
    REQUIRE(stream.str() == L("v1.2.3 by Hello World\r\n  No changelog\r\n"));
  }

  SECTION("with time") {
    ver.setTime("2016-01-02T00:42:11Z");
    stream << ver;
    REQUIRE(stream.str() == L("v1.2.3 \u2013 January 02 2016\r\n  No changelog\r\n"));
  }

  SECTION("with changelog") {
    ver.setChangelog(L("+ added super cool feature\n+ fixed all the bugs!"));
    stream << ver;
    REQUIRE(stream.str() == L("v1.2.3\r\n  + added super cool feature\r\n  + fixed all the bugs!\r\n"));
  }

  SECTION("changelog with empty lines") {
    ver.setChangelog(L("line1\n\nline2"));
    stream << ver; // no crash!
    REQUIRE(stream.str() == L("v1.2.3\r\n  line1\r\n\r\n  line2\r\n"));
  }
}
