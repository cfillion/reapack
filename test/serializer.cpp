#include <catch.hpp>

#include <serializer.hpp>

static const char *M = "[serializer]";

TEST_CASE("read from serialized data", M) {
  Serializer s;
  REQUIRE(s.userVersion() == 0);
  REQUIRE_FALSE(s);

  SECTION("valid") {
    const auto &out = s.read("1 1,1 2,3 4,5 6", 1);
    REQUIRE(out.size() == 3);
    REQUIRE(out[0] == (Serializer::Record{1,2}));
    REQUIRE(out[1] == (Serializer::Record{3,4}));
    REQUIRE(out[2] == (Serializer::Record{5,6}));

    REQUIRE(s.userVersion() == 1);
    REQUIRE(s);
  }

  SECTION("wrong user version") {
    const auto &out = s.read("1 1,1 2,3 4,5 6", 2);
    REQUIRE(out.empty());
    REQUIRE(s.userVersion() == 2);
    REQUIRE(s);
  }

  SECTION("wrong data version") {
    const auto &out = s.read("1 42,1 2,3 4,5 6", 1);
    REQUIRE(out.empty());
    REQUIRE(s.userVersion() == 1);
    REQUIRE(s);
  }

  SECTION("not an integer") {
    const auto &out = s.read("1 1,1 2,hello world,3 4", 1);
    REQUIRE(out.size() == 1);
    REQUIRE(out[0] == (Serializer::Record{1,2}));
  }

  SECTION("single field") {
    const auto &out = s.read("1 1,1 2,3,4 5", 1);
    REQUIRE(out.size() == 1);
    REQUIRE(out[0] == (Serializer::Record{1,2}));
  }

  SECTION("empty string") {
    const auto &out = s.read("", 1);
    REQUIRE(out.empty());
  }
}

TEST_CASE("write to string", M) {
  Serializer s;
  REQUIRE(s.write({{1, 2}}).empty()); // no user version set
  s.read({}, 42);

  REQUIRE(s.write({{1, 2}, {3, 4}}) == "42 1,1 2,3 4");
}
