#include <catch.hpp>

#include <remote.hpp>

using namespace std;

static const char *M = "[remote]";

TEST_CASE("construct remote", M) {
  Remote remote{"name", "url", true};
  REQUIRE(remote.name() == "name");
  REQUIRE(remote.url() == "url");
  REQUIRE(remote.isEnabled());
  REQUIRE_FALSE(remote.isFrozen());
}

TEST_CASE("null remote", M) {
  Remote remote;
  REQUIRE(remote.isNull());

  SECTION("set name") {
    remote.setName("test");
    REQUIRE(remote.name() == "test");
    REQUIRE(remote.isNull());
  }

  SECTION("set url") {
    remote.setUrl("test");
    REQUIRE(remote.url() == "test");
    REQUIRE(remote.isNull());
  }

  SECTION("set both") {
    remote.setName("hello");
    remote.setUrl("world");
    REQUIRE_FALSE(remote.isNull());
  }
}

TEST_CASE("freeze remote", M) {
  Remote remote;
  REQUIRE_FALSE(remote.isFrozen());

  remote.freeze();
  REQUIRE(remote.isFrozen());
}

TEST_CASE("add remotes to list", M) {
  RemoteList list;

  REQUIRE(list.empty());
  REQUIRE(list.size() == 0);
  REQUIRE_FALSE(list.hasName("name"));
  REQUIRE_FALSE(list.hasUrl("url"));

  list.add({"name", "url"});

  REQUIRE_FALSE(list.empty());
  REQUIRE(list.size() == 1);
  REQUIRE(list.hasName("name"));
  REQUIRE(list.hasUrl("url"));
}

TEST_CASE("replace remote", M) {
  RemoteList list;

  list.add({"name", "url1"});
  list.add({"name", "url2"});

  REQUIRE(list.size() == 1);
  REQUIRE_FALSE(list.hasUrl("url1"));
  REQUIRE(list.hasUrl("url2"));
};

TEST_CASE("get remote by name", M) {
  RemoteList list;
  REQUIRE(list.get("hello").isNull());

  list.add({"hello", "world"});
  REQUIRE_FALSE(list.get("hello").isNull());
}
