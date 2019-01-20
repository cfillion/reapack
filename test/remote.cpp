#include "helper.hpp"

#include <errors.hpp>
#include <remote.hpp>

using namespace std;

static const char *M = "[remote]";

TEST_CASE("construct remote", M) {
  Remote remote{"name", "url", true};
  REQUIRE(remote.name() == "name");
  REQUIRE(remote.url() == "url");
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
        Remote remote(name, "url");
        FAIL("'" + name + "' was allowed");
      }
      catch(const reapack_error &e) {
        REQUIRE(string(e.what()) == "invalid name");
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
        Remote remote(name, "url");
      }
      catch(const reapack_error &e) {
        FAIL("'" + name + "' was denied (" + e.what() + ')');
      }
    }
  }
}

TEST_CASE("remote url validation", M) {
  SECTION("invalid") {
    const string invalidUrls[] = {
      "",
      "hello world", // space should be %20
    };

    for(const string &url : invalidUrls) {
      try {
        Remote remote("hello", url);
        FAIL("'" + url + "' was allowed");
      }
      catch(const reapack_error &e) {
        REQUIRE(string(e.what()) == "invalid url");
      }
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

TEST_CASE("valid remote urls", M) {
  Remote remote;

  SECTION("uppercase")
    remote.setUrl("https://google.com/AAA");

  SECTION("escape sequence")
    remote.setUrl("https://google.com/?q=hello%20world");

  SECTION("libstdc++ bug #71500") // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=71500
    remote.setUrl("https://google.com/RRR");
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
  remote.setUrl("https://cfillion.ca");
  REQUIRE_FALSE(remote.isProtected());

  remote.protect();
  REQUIRE(remote.isProtected());

  try {
    remote.setUrl("https://google.com");
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "cannot change the URL of a protected repository");
  }

  remote.setUrl(remote.url()); // this should work for AddSetRepository API
}

TEST_CASE("autoinstall remote", M) {
  Remote remote;
  REQUIRE_FALSE(remote.autoInstall());
  REQUIRE(remote.autoInstall(true));
  REQUIRE_FALSE(remote.autoInstall(false));

  remote.setAutoInstall(true);
  REQUIRE(remote.autoInstall());
  REQUIRE(remote.autoInstall(true));
  REQUIRE(remote.autoInstall(false));

  remote.setAutoInstall(false);
  REQUIRE_FALSE(remote.autoInstall());
  REQUIRE_FALSE(remote.autoInstall(true));
  REQUIRE_FALSE(remote.autoInstall(false));
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
  SECTION("invalid name")
    REQUIRE_FALSE(Remote::fromString("&"));

  SECTION("invalid name")
    REQUIRE_FALSE(Remote::fromString("name||false"));

  SECTION("enabled") {
    Remote remote = Remote::fromString("name|url|1");
    REQUIRE(remote);

    REQUIRE(remote.name() == "name");
    REQUIRE(remote.url() == "url");
    REQUIRE(remote.isEnabled());
  }

  SECTION("disabled") {
    Remote remote = Remote::fromString("name|url|0");
    REQUIRE(remote.name() == "name");
    REQUIRE(remote.url() == "url");
    REQUIRE_FALSE(remote.isEnabled());
  }

  SECTION("missing auto-install") {
    Remote remote = Remote::fromString("name|url|1");
    REQUIRE(boost::logic::indeterminate(remote.autoInstall()));
  }

  SECTION("indeterminate auto-install") {
    Remote remote = Remote::fromString("name|url|1|2");
    REQUIRE(boost::logic::indeterminate(remote.autoInstall()));
  }

  SECTION("auto-install enabled") {
    Remote remote = Remote::fromString("name|url|1|1");
    REQUIRE(remote.autoInstall());
  }

  SECTION("auto-install enabled") {
    Remote remote = Remote::fromString("name|url|1|0");
    REQUIRE(bool{remote.autoInstall() == false});
  }
}

TEST_CASE("serialize remote", M) {
  SECTION("default")
    REQUIRE(Remote("name", "url").toString() == "name|url|1|2");

  SECTION("enabled")
    REQUIRE(Remote("name", "url", true, true).toString() == "name|url|1|1");

  SECTION("disabled")
    REQUIRE(Remote("name", "url", false, false).toString() == "name|url|0|0");
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
  REQUIRE(Remote("aaa", "aaa") < Remote("bbb", "aaa"));
  REQUIRE_FALSE(Remote("aaa", "aaa") < Remote("aaa", "bbb"));
}
