#include "helper.hpp"

#include <errors.hpp>
#include <remote.hpp>

using namespace std;

static const char *M = "[remote]";

TEST_CASE("construct remote (name)", M) {
  Remote remote("name");
  REQUIRE(remote.name() == "name");
  REQUIRE(remote.url() == "");
  REQUIRE(remote.isEnabled());
  REQUIRE(remote.flags() == 0);
}

TEST_CASE("construct remote (name, url)", M) {
  Remote remote("name", "url");
  REQUIRE(remote.name() == "name");
  REQUIRE(remote.url() == "url");
  REQUIRE(remote.isEnabled());
  REQUIRE(remote.flags() == 0);
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
  SECTION("name") {
    try {
      Remote r("/", "url");
      FAIL();
    }
    catch(const reapack_error &e) {
      REQUIRE(string(e.what()) == "invalid name");
    }
  }

  SECTION("url") {
    try {
      Remote r("ReaPack", "http://google.com/hello?invalid=|");
      FAIL();
    }
    catch(const reapack_error &e) {
      REQUIRE(string(e.what()) == "invalid url");
    }
  }
}

TEST_CASE("valid remote urls", M) {
  Remote remote("name");

  SECTION("uppercase")
    remote.setUrl("https://google.com/AAA");

  SECTION("escape sequence")
    remote.setUrl("https://google.com/?q=hello%20world");

  SECTION("libstdc++ bug #71500") // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=71500
    remote.setUrl("https://google.com/RRR");
}

TEST_CASE("protect remote", M) {
  Remote remote("name");
  REQUIRE_FALSE(remote.test(Remote::ProtectedFlag));
  remote.setUrl("https://google.com");

  remote.protect();
  REQUIRE(remote.test(Remote::ProtectedFlag));

  try {
    remote.setUrl("https://different.url");
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "cannot change the URL of a protected repository");
  }

  // setting the same url must work for the AddSetRepository API
  remote.setUrl(remote.url());
}

TEST_CASE("autoinstall remote", M) {
  Remote remote("name");
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
  RemotePtr remote = make_shared<Remote>("name");

  RemoteList list;
  REQUIRE(list.empty());
  REQUIRE(list.size() == 0);
  REQUIRE(list.getByName("name") == nullptr);

  list.add(remote);
  REQUIRE_FALSE(list.empty());
  REQUIRE(list.size() == 1);
  REQUIRE(list.getByName("name") == remote);

  list.add(remote);
  REQUIRE(list.size() == 1);
}

TEST_CASE("get no enabled remote", M) {
  RemotePtr r1 = make_shared<Remote>("hello"),
            r2 = make_shared<Remote>("world");

  RemoteList list;
  list.add(r1);
  list.add(r2);

  r1->setEnabled(false);
  r2->setEnabled(false);

  const auto &range = list.enabled();
  REQUIRE(range.empty());
}

TEST_CASE("get one enabled remote", M) {
  RemotePtr r1 = make_shared<Remote>("hello"),
            r2 = make_shared<Remote>("world");

  RemoteList list;
  list.add(r1);
  list.add(r2);

  r1->setEnabled(false);
  r2->setEnabled(true);

  const auto &range = list.enabled();
  REQUIRE(boost::size(range) == 1);
  REQUIRE(*range.begin() == r2);
}

TEST_CASE("get self remote", M) {
  RemotePtr self = make_shared<Remote>("ReaPack");

  RemoteList list;
  REQUIRE(list.getSelf() == nullptr);

  list.add(self);
  REQUIRE(list.getSelf() == self);
}

TEST_CASE("remove remote", M) {
  RemotePtr remote = make_shared<Remote>("name");

  RemoteList list;
  list.add(remote);
  REQUIRE(list.size() == 1);
  REQUIRE_FALSE(list.empty());

  list.remove(remote);
  REQUIRE(list.size() == 0);
  REQUIRE(list.empty());

  list.remove(remote); // no crash
}

TEST_CASE("unserialize remote", M) {
  SECTION("invalid name")
    REQUIRE_FALSE(Remote::fromString("&"));

  SECTION("invalid name")
    REQUIRE_FALSE(Remote::fromString("name||false"));

  SECTION("name & enabled") {
    RemotePtr remote = Remote::fromString("name|url|1");
    REQUIRE(remote);

    REQUIRE(remote->name() == "name");
    REQUIRE(remote->url() == "url");
    REQUIRE(remote->isEnabled());
  }

  SECTION("disabled") {
    RemotePtr remote = Remote::fromString("name|url|0");
    REQUIRE(remote->name() == "name");
    REQUIRE(remote->url() == "url");
    REQUIRE_FALSE(remote->isEnabled());
  }

  SECTION("missing auto-install (backward compatibility)") {
    RemotePtr remote = Remote::fromString("name|url|1");
    REQUIRE(boost::logic::indeterminate(remote->autoInstall()));
  }

  SECTION("indeterminate auto-install") {
    RemotePtr remote = Remote::fromString("name|url|1|2");
    REQUIRE(boost::logic::indeterminate(remote->autoInstall()));
  }

  SECTION("auto-install enabled") {
    RemotePtr remote = Remote::fromString("name|url|1|1");
    REQUIRE(remote->autoInstall());
  }

  SECTION("auto-install enabled") {
    RemotePtr remote = Remote::fromString("name|url|1|0");
    REQUIRE(remote->autoInstall() == false);
  }
}

TEST_CASE("serialize remote", M) {
  Remote r("name", "url");
  REQUIRE(r.toString() == "name|url|1|2");

  r.setAutoInstall(true);
  REQUIRE(r.toString() == "name|url|1|1");

  r.setEnabled(false);
  REQUIRE(r.toString() == "name|url|0|1");

  r.setAutoInstall(false);
  REQUIRE(r.toString() == "name|url|0|0");
}
