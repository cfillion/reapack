#include "helper.hpp"

#include <filter.hpp>

using namespace std;

static const char *M = "[filter]";

TEST_CASE("basic matching", M) {
  Filter f;
  REQUIRE(f.match({}));
  REQUIRE(f.match({L("world")}));

  f.set(L("hello"));

  REQUIRE(f.match({L("hello")}));
  REQUIRE(f.match({L("HELLO")}));
  REQUIRE_FALSE(f.match({L("world")}));
}

TEST_CASE("get/set filter", M) {
  Filter f;
  REQUIRE(f.get().empty());

  f.set(L("hello"));
  REQUIRE(f.get() == L("hello"));
  REQUIRE(f.match({L("hello")}));

  f.set(L("world"));
  REQUIRE(f.get() == L("world"));
  REQUIRE(f.match({L("world")}));
}

TEST_CASE("filter operators", M) {
  SECTION("assignment") {
    Filter f;
    f = L("hello");
    REQUIRE(f.get() == L("hello"));
  }

  SECTION("equal") {
    REQUIRE(Filter(L("hello")) == L("hello"));
    REQUIRE_FALSE(Filter(L("hello")) == L("world"));
  }

  SECTION("not equal") {
    REQUIRE_FALSE(Filter(L("hello")) != L("hello"));
    REQUIRE(Filter(L("hello")) != L("world"));
  }
}

TEST_CASE("word matching", M) {
  Filter f;
  f.set(L("hello world"));

  REQUIRE_FALSE(f.match({L("hello")}));
  REQUIRE(f.match({L("hello world")}));
  REQUIRE(f.match({L("helloworld")}));
  REQUIRE(f.match({L("hello test world")}));
}

TEST_CASE("quote phrase matching", M) {
  Filter f;

  SECTION("double quotes")
    f.set(L("\"hello world\""));
  SECTION("single quotes")
    f.set(L("'hello world'"));

  REQUIRE(f.match({L("hello world")}));
  REQUIRE(f.match({L("BEFOREhello worldAFTER")}));
  REQUIRE_FALSE(f.match({L("helloworld")}));
  REQUIRE_FALSE(f.match({L("hello test world")}));
}

TEST_CASE("quote word matching", M) {
  Filter f;

  SECTION("double quotes")
    f.set(L("\"word\""));
  SECTION("single quotes")
    f.set(L("'word'"));

  REQUIRE(f.match({L("BEFORE word AFTER")}));
  REQUIRE(f.match({L("_word_")}));
  REQUIRE_FALSE(f.match({L("BEFOREword")}));
  REQUIRE_FALSE(f.match({L("wordAFTER")}));
  REQUIRE_FALSE(f.match({L("BEFOREwordAFTER")}));
}

TEST_CASE("mixing quotes", M) {
  Filter f;

  SECTION("double in single") {
    f.set(L("'hello \"world\"'"));

    REQUIRE(f.match({L("hello \"world\"")}));
    REQUIRE_FALSE(f.match({L("hello world")}));
  }

  SECTION("single in double") {
    f.set(L("\"hello 'world'\""));

    REQUIRE(f.match({L("hello 'world'")}));
    REQUIRE_FALSE(f.match({L("hello world")}));
  }
}

TEST_CASE("start of string", M) {
  Filter f;

  SECTION("normal") {
    f.set(L("^hello"));

    REQUIRE(f.match({L("hello world")}));
    REQUIRE_FALSE(f.match({L("puts 'hello world'")}));
  }

  SECTION("middle") {
    f.set(L("hel^lo"));

    REQUIRE(f.match({L("hel^lo world")}));
    REQUIRE_FALSE(f.match({L("hello world")}));
  }

  SECTION("single") {
    f.set(L("^"));
    REQUIRE(f.match({L("hel^lo world")}));
    REQUIRE_FALSE(f.match({L("hello world")}));
  }

  SECTION("quote before") {
    f.set(L("'^hello'"));
    REQUIRE(f.match({L("hello world")}));
    REQUIRE_FALSE(f.match({L("world hello")}));
  }

  SECTION("quote after") {
    f.set(L("^'hello"));
    REQUIRE(f.match({L("hello world")}));
    REQUIRE_FALSE(f.match({L("world hello")}));
  }
}

TEST_CASE("end of string", M) {
  Filter f;

  SECTION("normal") {
    f.set(L("world$"));

    REQUIRE(f.match({L("hello world")}));
    REQUIRE_FALSE(f.match({L("'hello world'.upcase")}));
  }

  SECTION("middle") {
    f.set(L("hel$lo"));

    REQUIRE(f.match({L("hel$lo world")}));
    REQUIRE_FALSE(f.match({L("hello world")}));
  }

  SECTION("single") {
    f.set(L("$"));
    REQUIRE(f.match({L("hel$lo world")}));
    REQUIRE_FALSE(f.match({L("hello world")}));
  }

  SECTION("quote before") {
    f.set(L("'hello'$"));
    REQUIRE(f.match({L("hello")}));
    REQUIRE_FALSE(f.match({L("hello world")}));
  }

  SECTION("quote after") {
    f.set(L("'hello$'"));
    REQUIRE(f.match({L("hello")}));
    REQUIRE_FALSE(f.match({L("hello world")}));
  }
}

TEST_CASE("both anchors", M) {
  Filter f;
  f.set(L("^word$"));

  REQUIRE(f.match({L("word")}));
  REQUIRE_FALSE(f.match({L("word after")}));
  REQUIRE_FALSE(f.match({L("before word")}));
}

TEST_CASE("row matching", M) {
  Filter f;
  f.set(L("hello world"));

  REQUIRE_FALSE(f.match({L("hello")}));
  REQUIRE(f.match({L("hello"), L("world")}));
  REQUIRE(f.match({L("hello"), L("test"), L("world")}));
}

TEST_CASE("OR operator", M) {
  Filter f;

  SECTION("normal") {
    f.set(L("hello OR bacon"));

    REQUIRE(f.match({L("hello world")}));
    REQUIRE(f.match({L("chunky bacon")}));
    REQUIRE_FALSE(f.match({L("not matching")}));
    REQUIRE_FALSE(f.match({L("OR")}));
  }

  SECTION("anchor") {
    f.set(L("world OR ^bacon"));

    REQUIRE(f.match({L("hello world")}));
    REQUIRE_FALSE(f.match({L("chunky bacon")}));
    REQUIRE(f.match({L("bacon")}));
  }

  SECTION("quoted") {
    f.set(L("hello 'OR' bacon"));

    REQUIRE_FALSE(f.match({L("hello world")}));
    REQUIRE(f.match({L("hello OR bacon")}));
  }

  SECTION("reset") {
    f.set(L("hello OR bacon world"));

    REQUIRE_FALSE(f.match({L("world")}));
    REQUIRE(f.match({L("hello world")}));
    REQUIRE(f.match({L("bacon world")}));
  }

  SECTION("single") {
    f.set(L("OR"));
    REQUIRE(f.match({L("anything")}));
  }
}

TEST_CASE("NOT operator", M) {
  Filter f;

  SECTION("normal") {
    f.set(L("hello NOT bacon"));

    REQUIRE(f.match({L("hello world")}));
    REQUIRE_FALSE(f.match({L("chunky bacon")}));
    REQUIRE_FALSE(f.match({L("hello NOT bacon")}));
  }

  SECTION("row matching") {
    f.set(L("NOT bacon"));

    REQUIRE_FALSE(f.match({L("hello"), L("bacon"), L("world")}));
    REQUIRE(f.match({L("hello"), L("world")}));
  }

  SECTION("preceded by OR") {
    f.set(L("hello OR NOT bacon"));
    REQUIRE(f.match({L("hello bacon")}));
    REQUIRE(f.match({L("hello"), L("bacon")}));
  }

  SECTION("followed by OR") {
    f.set(L("NOT bacon OR hello"));
    REQUIRE(f.match({L("hello bacon")}));
    REQUIRE(f.match({L("hello"), L("bacon")}));
  }

  SECTION("quote word matching") {
    f.set(L("NOT 'hello'"));
    REQUIRE(f.match({L("hellobacon")}));
  }

  SECTION("NOT NOT") {
    f.set(L("NOT NOT hello"));
    REQUIRE(f.match({L("hello")}));
    REQUIRE_FALSE(f.match({L("world")}));
  }
}

TEST_CASE("AND grouping", M) {
  Filter f;

  SECTION("normal") {
    f.set(L("( hello world ) OR ( NOT hello bacon )"));

    REQUIRE(f.match({L("hello world")}));
    REQUIRE(f.match({L("chunky bacon")}));
    REQUIRE_FALSE(f.match({L("hello chunky bacon")}));
  }

  SECTION("close without opening") {
    f.set(L(") test"));
  }

  SECTION("NOT + AND grouping") {
    f.set(L("NOT ( apple orange ) bacon"));

    REQUIRE(f.match({L("bacon")}));
    REQUIRE(f.match({L("apple bacon")}));
    REQUIRE(f.match({L("orange bacon")}));
    REQUIRE_FALSE(f.match({L("apple bacon orange")}));
  }

  SECTION("NOT + AND + OR grouping") {
    f.set(L("NOT ( apple OR orange ) OR bacon"));

    REQUIRE_FALSE(f.match({L("apple")}));
    REQUIRE_FALSE(f.match({L("orange")}));
    REQUIRE(f.match({L("test")}));
    REQUIRE(f.match({L("apple bacon")}));
    REQUIRE(f.match({L("bacon")}));
  }

  SECTION("nested groups") {
    f.set(L("NOT ( ( apple OR orange ) OR bacon )"));

    REQUIRE_FALSE(f.match({L("apple")}));
    REQUIRE_FALSE(f.match({L("orange")}));
    REQUIRE(f.match({L("test")}));
    REQUIRE_FALSE(f.match({L("apple bacon")}));
    REQUIRE_FALSE(f.match({L("bacon")}));
  }
}
