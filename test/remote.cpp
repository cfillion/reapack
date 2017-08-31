#include "helper.hpp"

#include <errors.hpp>
#include <remote.hpp>

using namespace std;

static const char *M = "[remote]";

TEST_CASE("construct remote", M) {
  Remote remote{L("name"), L("url"), true};
  REQUIRE(remote.name() == L("name"));
  REQUIRE(remote.url() == L("url"));
  REQUIRE(remote.isEnabled());
  REQUIRE_FALSE(remote.isProtected());
}

TEST_CASE("remote name validation", M) {
  SECTION("invalid") {
    const string invalidNames[] = {
      "",
      "ab/cd",
      "ab\\cd",
      "..",
      ".",
      "....",
      ".hidden",
      "trailing.",
      "   leading",
      "trailing   ",
      "ctrl\4chars",

      // Windows device names...
      "CLOCK$",
      "COM1",
      "LPT2",
      "lpt1",
    };

    for(const string &name : invalidNames) {
      try {
        Remote remote(name, L("url"));
        FAIL("'" + name + "' was allowed");
      }
      catch(const reapack_error &e) {
        REQUIRE(e.what() == L("invalid name"));
      }
    }
  }

  SECTION("valid") {
    const string validNames[] = {
      "1234",
      "hello world",
      "hello_world",
      "Новая папка",
      "Hello ~World~",
      "Repository #1",
    };

    for(const string &name : validNames) {
      try {
        Remote remote(name, L("url"));
      }
      catch(const reapack_error &e) {
        FAIL("'" + name + "' was denied (" + (string)e.what() + ')');
      }
    }
  }
}

TEST_CASE("remote url validation", M) {
  SECTION("invalid") {
    const string invalidUrls[] = {
      {}, // empty string
      "hello world", // space should be %20
    };

    for(const string &url : invalidUrls) {
      try {
        Remote remote(L("hello"), url);
        FAIL("'" + url + "' was allowed");
      }
      catch(const reapack_error &e) {
        REQUIRE(e.what() == L("invalid url"));
      }
    }
  }
}

TEST_CASE("set invalid values", M) {
  Remote remote;
  
  SECTION("name") {
    try {
      remote.setName(L("/"));
      FAIL();
    }
    catch(const reapack_error &e) {
      REQUIRE(e.what() == L("invalid name"));
    }
  }

  SECTION("url") {
    try {
      remote.setUrl(L("http://google.com/hello?invalid=|"));
      FAIL();
    }
    catch(const reapack_error &e) {
      REQUIRE(e.what() == L("invalid url"));
    }
  }
}

TEST_CASE("valide remote urls", M) {
  Remote remote;

  SECTION("uppercase")
    remote.setUrl(L("https://google.com/AAA"));

  SECTION("escape sequence")
    remote.setUrl(L("https://google.com/?q=hello%20world"));

  SECTION("libstdc++ bug #71500") // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=71500
    remote.setUrl(L("https://google.com/RRR"));
}

TEST_CASE("null remote", M) {
  Remote remote;
  REQUIRE(remote.isNull());
  REQUIRE_FALSE(remote);
  CHECK(remote.isEnabled());

  SECTION("set name") {
    remote.setName(L("test"));
    REQUIRE(remote.name() == L("test"));
    REQUIRE(remote.isNull());
  }

  SECTION("set url") {
    remote.setUrl(L("test"));
    REQUIRE(remote.url() == L("test"));
    REQUIRE(remote.isNull());
  }

  SECTION("set both") {
    remote.setName(L("hello"));
    remote.setUrl(L("world"));
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
  REQUIRE_FALSE(list.hasName(L("name")));

  list.add({L("name"), L("url")});

  REQUIRE_FALSE(list.empty());
  REQUIRE(list.size() == 1);
  REQUIRE(list.hasName(L("name")));
}

TEST_CASE("add invalid remote to list", M) {
  RemoteList list;
  list.add({});

  REQUIRE(list.empty());
}

TEST_CASE("replace remote", M) {
  RemoteList list;

  list.add({L("name"), L("url1")});
  list.add({L("name"), L("url2")});

  REQUIRE(list.size() == 1);
  REQUIRE(list.get(L("name")).url() == L("url2"));
};

TEST_CASE("get remote by name", M) {
  RemoteList list;
  REQUIRE(list.get(L("hello")).isNull());

  list.add({L("hello"), L("world")});
  REQUIRE_FALSE(list.get(L("hello")).isNull());
}

TEST_CASE("unserialize remote", M) {
  SECTION("invalid name")
    REQUIRE_FALSE(Remote::fromString(L("&")));

  SECTION("invalid name")
    REQUIRE_FALSE(Remote::fromString(L("name||false")));

  SECTION("enabled") {
    Remote remote = Remote::fromString(L("name|url|1"));
    REQUIRE(remote);

    REQUIRE(remote.name() == L("name"));
    REQUIRE(remote.url() == L("url"));
    REQUIRE(remote.isEnabled());
  }

  SECTION("disabled") {
    Remote remote = Remote::fromString(L("name|url|0"));
    REQUIRE(remote.name() == L("name"));
    REQUIRE(remote.url() == L("url"));
    REQUIRE_FALSE(remote.isEnabled());
  }

  SECTION("missing auto-install") {
    Remote remote = Remote::fromString(L("name|url|1"));
    REQUIRE(boost::logic::indeterminate(remote.autoInstall()));
  }

  SECTION("indeterminate auto-install") {
    Remote remote = Remote::fromString(L("name|url|1|2"));
    REQUIRE(boost::logic::indeterminate(remote.autoInstall()));
  }

  SECTION("auto-install enabled") {
    Remote remote = Remote::fromString(L("name|url|1|1"));
    REQUIRE(remote.autoInstall());
  }

  SECTION("auto-install enabled") {
    Remote remote = Remote::fromString(L("name|url|1|0"));
    REQUIRE(remote.autoInstall() == false);
  }
}

TEST_CASE("serialize remote", M) {
  SECTION("default")
    REQUIRE(Remote(L("name"), L("url")).toString() == L("name|url|1|2"));

  SECTION("enabled")
    REQUIRE(Remote(L("name"), L("url"), true, true).toString() == L("name|url|1|1"));

  SECTION("disabled")
    REQUIRE(Remote(L("name"), L("url"), false, false).toString() == L("name|url|0|0"));
}

TEST_CASE("get enabled remotes", M) {
  RemoteList list;
  list.add({L("hello"), L("url1"), true});
  list.add({L("world"), L("url2"), false});

  const vector<Remote> array = list.getEnabled();

  REQUIRE(array.size() == 1);
  REQUIRE(array[0].name() == L("hello"));
}

TEST_CASE("remove remote", M) {
  const Remote remote{L("hello"), L("url")};
  RemoteList list;

  list.add(remote);
  REQUIRE(list.size() == 1);
  REQUIRE_FALSE(list.empty());

  list.remove(remote);
  REQUIRE(list.size() == 0);
  REQUIRE(list.empty());

  list.remove(L("world")); // no crash
}

TEST_CASE("remove two remotes", M) {
  RemoteList list;

  list.add({L("a_first"), L("url")});
  list.add({L("z_second"), L("url")});
  list.add({L("b_third"), L("url")});
  REQUIRE(list.size() == 3);

  list.remove(L("z_second"));
  REQUIRE(list.size() == 2);

  REQUIRE(list.get(L("z_first")).isNull());
  REQUIRE_FALSE(list.get(L("b_third")).isNull());
  REQUIRE_FALSE(list.get(L("a_first")).isNull());
}

TEST_CASE("compare remotes", M) {
  REQUIRE(Remote(L("aaa"), L("aaa")) < Remote(L("bbb"), L("aaa")));
  REQUIRE_FALSE(Remote(L("aaa"), L("aaa")) < Remote(L("aaa"), L("bbb")));
}
