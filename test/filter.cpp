#include "helper.hpp"

#include <filter.hpp>

static const char *M = "[filter]";

TEST_CASE("basic matching", M) {
  Filter f;
  REQUIRE(f.match({}));
  REQUIRE(f.match({"world"}));

  f.set("hello");
  REQUIRE(f.match({"hello"}));
  REQUIRE(f.match({"HELLO"}));
  REQUIRE_FALSE(f.match({"world"}));
}

TEST_CASE("word matching", M) {
  Filter f;
  f.set("hello world");

  REQUIRE_FALSE(f.match({"hello"}));
  REQUIRE(f.match({"hello world"}));
  REQUIRE(f.match({"helloworld"}));
  REQUIRE(f.match({"hello test world"}));
}

TEST_CASE("quote phrase matching", M) {
  Filter f;

  SECTION("double quotes")
    f.set("\"foo bar\" baz");
  SECTION("single quotes")
    f.set("'foo bar' baz");

  REQUIRE(f.match({"baz foo bar"}));
  REQUIRE(f.match({"BEFOREfoo barAFTER baz"}));
  REQUIRE_FALSE(f.match({"foobarbaz"}));
  REQUIRE_FALSE(f.match({"foo test bar baz"}));
}

TEST_CASE("full word matching", M) {
  Filter f;

  SECTION("double quotes")
    f.set("\"hello\" world");
  SECTION("single quotes")
    f.set("'hello' world");

  REQUIRE(f.match({"BEFORE hello AFTER world"}));
  REQUIRE(f.match({"_hello_ world"}));
  REQUIRE_FALSE(f.match({"BEFOREhello world"}));
  REQUIRE_FALSE(f.match({"helloAFTER world"}));
  REQUIRE_FALSE(f.match({"BEFOREhelloAFTER world"}));
}

TEST_CASE("late opening quote", M) {
  Filter f;
  f.set("foo'bar'");

  REQUIRE(f.match({"foo'bar'"}));
  REQUIRE_FALSE(f.match({"foo bar"}));
}

TEST_CASE("early closing quote", M) {
  Filter f;
  f.set("'foo'bar");

  REQUIRE(f.match({"foo bar"}));
  REQUIRE_FALSE(f.match({"foobar"}));
  REQUIRE_FALSE(f.match({"foo ar"}));
}

TEST_CASE("mixing quotes", M) {
  Filter f;

  SECTION("double in single") {
    f.set("'hello \"world\"'");

    REQUIRE(f.match({"hello \"world\""}));
    REQUIRE_FALSE(f.match({"hello world"}));
  }

  SECTION("single in double") {
    f.set("\"hello 'world'\"");

    REQUIRE(f.match({"hello 'world'"}));
    REQUIRE_FALSE(f.match({"hello world"}));
  }
}

TEST_CASE("start of string", M) {
  Filter f;

  SECTION("normal") {
    f.set("^hello");

    REQUIRE(f.match({"hello world"}));
    REQUIRE_FALSE(f.match({"puts 'hello world'"}));
  }

  SECTION("middle") {
    f.set("hel^lo");

    REQUIRE(f.match({"hel^lo world"}));
    REQUIRE_FALSE(f.match({"hello world"}));
  }

  SECTION("single") {
    f.set("^");
    REQUIRE(f.match({"hello world"}));
  }

  SECTION("literal ^") {
    f.set("'^hello'");
    REQUIRE(f.match({"^hello world"}));
    REQUIRE_FALSE(f.match({"world hello"}));
  }

  SECTION("full word") {
    f.set("^'hello");
    REQUIRE(f.match({"hello world"}));
    REQUIRE_FALSE(f.match({"world hello"}));
  }
}

TEST_CASE("end of string", M) {
  Filter f;

  SECTION("normal") {
    f.set("world$");

    REQUIRE(f.match({"hello world"}));
    REQUIRE_FALSE(f.match({"'hello world'.upcase"}));
  }

  SECTION("middle") {
    f.set("hel$lo");

    REQUIRE(f.match({"hel$lo world"}));
    REQUIRE_FALSE(f.match({"hello world"}));
  }

  SECTION("single") {
    f.set("$");
    REQUIRE(f.match({"hello world"}));
  }

  SECTION("full word") {
    f.set("'hello'$");
    REQUIRE(f.match({"hello"}));
    REQUIRE_FALSE(f.match({"hello world"}));
  }

  SECTION("literal $") {
    f.set("'hello$'");
    REQUIRE(f.match({"hello$"}));
    REQUIRE_FALSE(f.match({"hello world"}));
  }
}

TEST_CASE("both anchors", M) {
  Filter f;
  f.set("^word$");

  REQUIRE(f.match({"word"}));
  REQUIRE_FALSE(f.match({"word after"}));
  REQUIRE_FALSE(f.match({"before word"}));
}

TEST_CASE("row matching", M) {
  Filter f;
  f.set("hello world");

  REQUIRE_FALSE(f.match({"hello"}));
  REQUIRE(f.match({"hello", "world"}));
  REQUIRE(f.match({"hello", "test", "world"}));
}

TEST_CASE("OR operator", M) {
  Filter f;

  SECTION("normal") {
    f.set("hello OR bacon");

    REQUIRE(f.match({"hello world"}));
    REQUIRE(f.match({"chunky bacon"}));
    REQUIRE_FALSE(f.match({"not matching"}));
    REQUIRE_FALSE(f.match({"OR"}));
  }

  SECTION("anchor") {
    f.set("world OR ^bacon");

    REQUIRE(f.match({"hello world"}));
    REQUIRE_FALSE(f.match({"chunky bacon"}));
    REQUIRE(f.match({"bacon"}));
  }

  SECTION("reset") {
    f.set("hello OR bacon world");

    REQUIRE_FALSE(f.match({"world"}));
    REQUIRE(f.match({"hello world"}));
    REQUIRE(f.match({"bacon world"}));
  }

  SECTION("single") {
    f.set("OR");
    REQUIRE(f.match({"anything"}));
  }

  SECTION("literal OR") {
    f.set("'OR'");
    REQUIRE(f.match({"OR"}));
    REQUIRE_FALSE(f.match({"foo"}));
  }
}

TEST_CASE("NOT operator", M) {
  Filter f;

  SECTION("normal") {
    f.set("hello NOT bacon");

    REQUIRE(f.match({"hello world"}));
    REQUIRE_FALSE(f.match({"chunky bacon"}));
    REQUIRE_FALSE(f.match({"hello NOT bacon"}));
  }

  SECTION("row matching") {
    f.set("NOT bacon");

    REQUIRE_FALSE(f.match({"hello", "bacon", "world"}));
    REQUIRE(f.match({"hello", "world"}));
  }

  SECTION("preceded by OR") {
    f.set("hello OR NOT bacon");
    REQUIRE(f.match({"hello bacon"}));
    REQUIRE(f.match({"hello", "bacon"}));
  }

  SECTION("followed by OR") {
    f.set("NOT bacon OR hello");
    REQUIRE(f.match({"hello bacon"}));
    REQUIRE(f.match({"hello", "bacon"}));
  }

  SECTION("full word matching") {
    f.set("NOT 'hello'");
    REQUIRE(f.match({"hellobacon"}));
  }

  SECTION("NOT NOT") {
    f.set("NOT NOT hello");
    REQUIRE(f.match({"hello"}));
    REQUIRE_FALSE(f.match({"world"}));
  }

  SECTION("literal NOT") {
    f.set("'NOT'");
    REQUIRE(f.match({"NOT"}));
    REQUIRE_FALSE(f.match({"foo"}));
  }
}

TEST_CASE("AND grouping", M) {
  Filter f;

  SECTION("normal") {
    f.set("( hello world ) OR ( NOT hello bacon )");

    REQUIRE(f.match({"hello world"}));
    REQUIRE(f.match({"chunky bacon"}));
    REQUIRE_FALSE(f.match({"hello chunky bacon"}));
  }

  SECTION("close without opening") {
    f.set(") test");
  }

  SECTION("NOT + AND grouping") {
    f.set("NOT ( apple orange ) bacon");

    REQUIRE(f.match({"bacon"}));
    REQUIRE(f.match({"apple bacon"}));
    REQUIRE(f.match({"orange bacon"}));
    REQUIRE_FALSE(f.match({"apple bacon orange"}));
  }

  SECTION("NOT + AND + OR grouping") {
    f.set("NOT ( apple OR orange ) OR bacon");

    REQUIRE_FALSE(f.match({"apple"}));
    REQUIRE_FALSE(f.match({"orange"}));
    REQUIRE(f.match({"test"}));
    REQUIRE(f.match({"apple bacon"}));
    REQUIRE(f.match({"bacon"}));
  }

  SECTION("nested groups") {
    f.set("NOT ( ( apple OR orange ) OR bacon )");

    REQUIRE_FALSE(f.match({"apple"}));
    REQUIRE_FALSE(f.match({"orange"}));
    REQUIRE(f.match({"test"}));
    REQUIRE_FALSE(f.match({"apple bacon"}));
    REQUIRE_FALSE(f.match({"bacon"}));
  }

  SECTION("literal parentheses") {
    f.set("'('");
    REQUIRE(f.match({"("}));
    REQUIRE_FALSE(f.match({"foo"}));
  }

  SECTION("closing a subgroup rewinds to the AND parent") {
    f.set("a OR ( b ) c");

    REQUIRE_FALSE(f.match({"c"}));
    REQUIRE(f.match({"a c"}));
    REQUIRE(f.match({"b c"}));
  }

  SECTION("closing a subgroup doesn't rewind too far") {
    f.set("NOT ( a OR ( b ) c ) OR d");

    REQUIRE_FALSE(f.match({"a c"}));
    REQUIRE(f.match({"a z"}));
    REQUIRE(f.match({"d"}));
  }
}

TEST_CASE("synonymous words", M) {
  Filter f;

  SECTION("basic") {
    f.set("open");
    REQUIRE(f.match({"open"}));
    REQUIRE(f.match({"display"}));
    REQUIRE_FALSE(f.match({"door"}));
  }

  SECTION("case-insensitive") {
    f.set("OPEN");
    REQUIRE(f.match({"opening"}));
    REQUIRE(f.match({"display"}));
  }

  SECTION("full-word synonyms") {
    f.set("unselect");
    REQUIRE(f.match({"unselect"}));
    REQUIRE(f.match({"unselected"}));
    REQUIRE(f.match({"deselect"}));
    REQUIRE_FALSE(f.match({"deselected"}));
  }

  SECTION("preserve anchor flags") {
    f.set("^insert");
    REQUIRE(f.match({"add"}));
    REQUIRE_FALSE(f.match({"don't add"}));
    REQUIRE_FALSE(f.match({"not inserting things"}));
  }

  SECTION("NOT applies to all synonyms") {
    f.set("NOT open");
    REQUIRE(f.match({"foo bar"}));
    REQUIRE_FALSE(f.match({"open display"}));
  }

  SECTION("clear flags for the next token") {
    f.set("^open bar");
    REQUIRE(f.match({"open bar"}));
  }
}
