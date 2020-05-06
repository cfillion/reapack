#include "helper.hpp"

#include <string.hpp>

static const char *M = "[string]";

TEST_CASE("string format", M) {
  const std::string &formatted = String::format("%d%% Hello %s!", 100, "World");
  CHECK(formatted.size() == 17);
  REQUIRE(formatted == "100% Hello World!");
}

TEST_CASE("indent string", M) {
  std::string actual;

  SECTION("simple")
    actual = String::indent("line1\nline2");

  SECTION("already indented")
    actual = String::indent("  line1\n  line2");

  REQUIRE(actual == "  line1\r\n  line2");
}

TEST_CASE("pretty-print numbers", M) {
  REQUIRE(String::number(42'000'000) == "42,000,000");
}

TEST_CASE("strip RTF header", M) {
  REQUIRE("Hello World" == String::stripRtf(R"(
    {\rtf1\ansi{\fonttbl\f0\fswiss Helvetica;}\f0\pard
    Hello World
    }
  )"));
}

TEST_CASE("strip RTF color header", M) {
  REQUIRE("Hello World" == String::stripRtf(R"(
    {\rtf1\ansi\deff0{\fonttbl{\f0 \fswiss Helvetica;}{\f1 Courier;}}
    {\colortbl;\red255\green0\blue0;\red0\green0\blue255;}
    \widowctrl\hyphauto
    Hello World
    }
  )"));
}

TEST_CASE("strip RTF paragraphs", M) {
  REQUIRE("foo\n\nbar\n\nbaz" == String::stripRtf(R"(
    {\pard \ql \f0 \sa180 \li0 \fi0 \b \fs36 foo\par}
    {\pard \ql \f0 \sa180 \li0 \fi0 \b \fs36 bar\par}
    {\pard \ql \f0 \sa180 \li0 \fi0 \b \fs36 baz\par}
  )"));
}

TEST_CASE("strip RTF line breaks", M) {
  REQUIRE("foo\nbar\nbaz" == String::stripRtf(R"(foo\line bar\line
  baz\line)"));
}

TEST_CASE("strip RTF literal braces", M) {
  REQUIRE("{ }" == String::stripRtf(R"(\{ \})"));
}
