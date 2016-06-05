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
      Remote remote("a/", "url");
      FAIL();
    }
    catch(const reapack_error &e) {
      REQUIRE(string(e.what()) == "invalid name");
    }
  }

  SECTION("directory traversal in name") {
    try {
      Remote remote("..", "url");
      FAIL("dotdot was allowed");
    }
    catch(const reapack_error &e) {
      REQUIRE(string(e.what()) == "invalid name");
    }

    try {
      Remote remote(".", "url");
      FAIL("single dot was allowed");
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

  SECTION("invalid url") {
    try {
      Remote remote("name", "hello world");
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
      remote.setUrl("http://google.com/hello?invalid=|");
      FAIL();
    }
    catch(const reapack_error &e) {
      REQUIRE(string(e.what()) == "invalid url");
    }
  }
}

TEST_CASE("valide remote urls", M) {
  Remote remote;

  SECTION("uppercase")
    remote.setUrl("https://google.com/AAA");

  SECTION("escape sequence")
    remote.setUrl("https://google.com/?q=hello%20world");
}

TEST_CASE("null remote", M) {
  Remote remote;
  REQUIRE(remote.isNull());
  REQUIRE_FALSE(remote);
  CHECK(remote.isEnabled());

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
    REQUIRE(remote);
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

  list.add({"name", "url"});

  REQUIRE_FALSE(list.empty());
  REQUIRE(list.size() == 1);
  REQUIRE(list.hasName("name"));
}

TEST_CASE("add invalid remote to list", M) {
  RemoteList list;
  list.add({});

  REQUIRE(list.empty());
}

TEST_CASE("replace remote", M) {
  RemoteList list;

  list.add({"name", "url1"});
  list.add({"name", "url2"});

  REQUIRE(list.size() == 1);
  REQUIRE(list.get("name").url() == "url2");
};

TEST_CASE("get remote by name", M) {
  RemoteList list;
  REQUIRE(list.get("hello").isNull());

  list.add({"hello", "world"});
  REQUIRE_FALSE(list.get("hello").isNull());
}

TEST_CASE("unserialize remote", M) {
  SECTION("invalid name") {
    Remote::ReadCode code;
    REQUIRE(Remote::fromString("&", &code).isNull());
    REQUIRE(code == Remote::InvalidName);
  }

  SECTION("invalid name") {
    Remote::ReadCode code;
    REQUIRE(Remote::fromString("name||false", &code).isNull());
    REQUIRE(code == Remote::InvalidUrl);
  }

  SECTION("valid enabled") {
    Remote::ReadCode code;
    Remote remote = Remote::fromString("name|url|1", &code);
    REQUIRE(code == Remote::Success);

    REQUIRE(remote.name() == "name");
    REQUIRE(remote.url() == "url");
    REQUIRE(remote.isEnabled());
  }

  SECTION("valid disabled") {
    Remote remote = Remote::fromString("name|url|0");
    REQUIRE(remote.name() == "name");
    REQUIRE(remote.url() == "url");
    REQUIRE_FALSE(remote.isEnabled());
  }
}

TEST_CASE("serialize remote", M) {
  SECTION("enabled") {
    REQUIRE(Remote("name", "url", true).toString() == "name|url|1");
  }

  SECTION("disabled") {
    REQUIRE(Remote("name", "url", false).toString() == "name|url|0");
  }
}

TEST_CASE("get enabled remotes", M) {
  RemoteList list;
  list.add({"hello", "url1", true});
  list.add({"world", "url2", false});

  const vector<Remote> array = list.getEnabled();

  REQUIRE(array.size() == 1);
  REQUIRE(array[0].name() == "hello");
}

TEST_CASE("remove remote", M) {
  const Remote remote{"hello", "url"};
  RemoteList list;

  list.add(remote);
  REQUIRE(list.size() == 1);
  REQUIRE_FALSE(list.empty());

  list.remove(remote);
  REQUIRE(list.size() == 0);
  REQUIRE(list.empty());

  list.remove("world"); // no crash
}

TEST_CASE("remove two remotes", M) {
  RemoteList list;

  list.add({"a_first", "url"});
  list.add({"z_second", "url"});
  list.add({"b_third", "url"});
  REQUIRE(list.size() == 3);

  list.remove("z_second");
  REQUIRE(list.size() == 2);

  REQUIRE(list.get("z_first").isNull());
  REQUIRE_FALSE(list.get("b_third").isNull());
  REQUIRE_FALSE(list.get("a_first").isNull());
}

TEST_CASE("compare remotes", M) {
  REQUIRE(Remote("a", "a") < Remote("b", "a"));
  REQUIRE_FALSE(Remote("a", "a") < Remote("a", "b"));
}
