#include <catch.hpp>

#include <filter.hpp>

using namespace std;

static const char *M = "[filter]";

TEST_CASE("basic matching", M) {
  Filter f;
  REQUIRE(f.match("world"));

  f.set("hello");

  REQUIRE(f.match("hello"));
  REQUIRE(f.match("HELLO"));
  REQUIRE_FALSE(f.match("world"));
}

TEST_CASE("get/set filter", M) {
  Filter f;
  REQUIRE(f.get().empty());

  f.set("hello");
  REQUIRE(f.get() == "hello");
  REQUIRE(f.match("hello"));

  f.set("world");
  REQUIRE(f.get() == "world");
  REQUIRE(f.match("world"));
}

TEST_CASE("word matching", M) {
  Filter f;
  f.set("hello world");

  REQUIRE_FALSE(f.match("hello"));
  REQUIRE(f.match("hello world"));
  REQUIRE(f.match("helloworld"));
  REQUIRE(f.match("hello test world"));
}

TEST_CASE("double quote matching", M) {
  Filter f;
  f.set("\"hello world\"");

  REQUIRE(f.match("hello world"));
  REQUIRE_FALSE(f.match("helloworld"));
  REQUIRE_FALSE(f.match("hello test world"));
}

TEST_CASE("single quote matching", M) {
  Filter f;
  f.set("'hello world'");

  REQUIRE(f.match("hello world"));
  REQUIRE_FALSE(f.match("helloworld"));
  REQUIRE_FALSE(f.match("hello test world"));
}

TEST_CASE("mixing quotes", M) {
  Filter f;

  SECTION("double in single") {
    f.set("'hello \"world\"'");

    REQUIRE(f.match("hello \"world\""));
    REQUIRE_FALSE(f.match("hello world"));
  }

  SECTION("single in double") {
    f.set("\"hello 'world'\"");

    REQUIRE(f.match("hello 'world'"));
    REQUIRE_FALSE(f.match("hello world"));
  }
}
