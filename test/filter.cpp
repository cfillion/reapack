#include "helper.hpp"

#include <filter.hpp>

using namespace std;

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

TEST_CASE("get/set filter", M) {
  Filter f;
  REQUIRE(f.get().empty());

  f.set("hello");
  REQUIRE(f.get() == "hello");
  REQUIRE(f.match({"hello"}));

  f.set("world");
  REQUIRE(f.get() == "world");
  REQUIRE(f.match({"world"}));
}

TEST_CASE("filter operators", M) {
  SECTION("assignment") {
    Filter f;
    f = "hello";
    REQUIRE(f.get() == "hello");
  }

  SECTION("equal") {
    REQUIRE(Filter("hello") == "hello");
    REQUIRE_FALSE(Filter("hello") == "world");
  }

  SECTION("not equal") {
    REQUIRE_FALSE(Filter("hello") != "hello");
    REQUIRE(Filter("hello") != "world");
  }
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
    f.set("\"hello world\"");
  SECTION("single quotes")
    f.set("'hello world'");

  REQUIRE(f.match({"hello world"}));
  REQUIRE(f.match({"BEFOREhello worldAFTER"}));
  REQUIRE_FALSE(f.match({"helloworld"}));
  REQUIRE_FALSE(f.match({"hello test world"}));
}

TEST_CASE("quote word matching", M) {
  Filter f;

  SECTION("double quotes")
    f.set("\"word\"");
  SECTION("single quotes")
    f.set("'word'");

  REQUIRE(f.match({"BEFORE word AFTER"}));
  REQUIRE(f.match({"_word_"}));
  REQUIRE_FALSE(f.match({"BEFOREword"}));
  REQUIRE_FALSE(f.match({"wordAFTER"}));
  REQUIRE_FALSE(f.match({"BEFOREwordAFTER"}));
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
    REQUIRE(f.match({"hel^lo world"}));
    REQUIRE_FALSE(f.match({"hello world"}));
  }

  SECTION("quote before") {
    f.set("'^hello'");
    REQUIRE(f.match({"hello world"}));
    REQUIRE_FALSE(f.match({"world hello"}));
  }

  SECTION("quote after") {
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
    REQUIRE(f.match({"hel$lo world"}));
    REQUIRE_FALSE(f.match({"hello world"}));
  }

  SECTION("quote before") {
    f.set("'hello'$");
    REQUIRE(f.match({"hello"}));
    REQUIRE_FALSE(f.match({"hello world"}));
  }

  SECTION("quote after") {
    f.set("'hello$'");
    REQUIRE(f.match({"hello"}));
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

  SECTION("quoted") {
    f.set("hello 'OR' bacon");

    REQUIRE_FALSE(f.match({"hello world"}));
    REQUIRE(f.match({"hello OR bacon"}));
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

  SECTION("quote word matching") {
    f.set("NOT 'hello'");
    REQUIRE(f.match({"hellobacon"}));
  }

  SECTION("NOT NOT") {
    f.set("NOT NOT hello");
    REQUIRE(f.match({"hello"}));
    REQUIRE_FALSE(f.match({"world"}));
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
}
