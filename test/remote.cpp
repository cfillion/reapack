#include <catch.hpp>

#include <remote.hpp>

#include <errors.hpp>

#define RPATH "test/remote/"

using namespace std;

static const char *M = "[remote]";

TEST_CASE("construct remote", M) {
  Remote remote{"name", "url", true};
  REQUIRE(remote.name() == "name");
  REQUIRE(remote.url() == "url");
  REQUIRE(remote.isEnabled());
  REQUIRE_FALSE(remote.isProtected());
}

TEST_CASE("construct invalid remote", M) {
  SECTION("empty name") {
    try {
      Remote remote({}, "url");
      FAIL();
    }
    catch(const reapack_error &e) {
      REQUIRE(string(e.what()) == "invalid name");
    }
  }

  SECTION("invalid name") {
    try {
      Remote remote("/", "url");
      FAIL();
    }
    catch(const reapack_error &e) {
      REQUIRE(string(e.what()) == "invalid name");
    }
  }

  SECTION("empty url") {
    try {
      Remote remote("name", {});
      FAIL();
    }
    catch(const reapack_error &e) {
      REQUIRE(string(e.what()) == "invalid url");
    }
  }
}

TEST_CASE("set invalid values", M) {
  Remote remote;
  
  SECTION("name") {
    try {
      remote.setName("/");
      FAIL();
    }
    catch(const reapack_error &e) {
      REQUIRE(string(e.what()) == "invalid name");
    }
  }

  SECTION("url") {
    try {
      remote.setUrl({});
      FAIL();
    }
    catch(const reapack_error &e) {
      REQUIRE(string(e.what()) == "invalid url");
    }
  }
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

TEST_CASE("protect remote", M) {
  Remote remote;
  REQUIRE_FALSE(remote.isProtected());

  remote.protect();
  REQUIRE(remote.isProtected());
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

TEST_CASE("remote from file", M) {
  Remote remote;

  SECTION("not found") {
    const auto code = Remote::fromFile(RPATH "404.ReaPackRemote", &remote);
    REQUIRE(code == Remote::ReadFailure);
    REQUIRE_FALSE(remote.isValid());
  }

  SECTION("valid") {
    const auto code = Remote::fromFile(RPATH "test.ReaPackRemote", &remote);
    REQUIRE(code == Remote::Success);
    REQUIRE(remote.isValid());
    REQUIRE(remote.name() == "name");
    REQUIRE(remote.url() == "url");
  }

  SECTION("invalid name") {
    const auto code = Remote::fromFile(
      RPATH "invalid_name.ReaPackRemote", &remote);

    REQUIRE(code == Remote::InvalidName);
    REQUIRE_FALSE(remote.isValid());
  }

  SECTION("invalid url") {
    const auto code = Remote::fromFile(
      RPATH "missing_url.ReaPackRemote", &remote);

    REQUIRE(code == Remote::InvalidUrl);
    REQUIRE_FALSE(remote.isValid());
  }

  SECTION("unicode name") {
    const auto code = Remote::fromFile(
      RPATH "Новая папка.ReaPackRemote", &remote);

    REQUIRE(code == Remote::Success);
  }
};
