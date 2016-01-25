#include <catch.hpp>

#include "helper/io.hpp"

#include <path.hpp>

using namespace std;

static const char *M = "[path]";

TEST_CASE("compare paths", M) {
  const Path a = Path("hello");
  const Path b = Path("world");

  REQUIRE_FALSE(a == b);
  REQUIRE(a != b);

  REQUIRE(a == a);
  REQUIRE_FALSE(a != a);
}

TEST_CASE("prepend and append path components", M) {
  Path path;
  REQUIRE(path.empty());
  REQUIRE(path.size() == 0);
  REQUIRE(path.join() == string());
  REQUIRE(path.dirname() == string());
  REQUIRE(path.basename() == string());

  path.prepend("world");
  REQUIRE_FALSE(path.empty());
  REQUIRE(path.size() == 1);
  REQUIRE(path.join() == "world");
  REQUIRE(path.dirname() == string());
  REQUIRE(path.basename() == "world");

  path.prepend("hello");
  REQUIRE(path.size() == 2);
#ifndef _WIN32
  REQUIRE(path.join() == "hello/world");
#else
  REQUIRE(path.join() == "hello\\world");
#endif
  REQUIRE(path.dirname() == "hello");
  REQUIRE(path.basename() == "world");

  path.append("test");
  REQUIRE(path.size() == 3);
#ifndef _WIN32
  REQUIRE(path.join() == "hello/world/test");
  REQUIRE(path.dirname() == "hello/world");
#else
  REQUIRE(path.join() == "hello\\world\\test");
  REQUIRE(path.dirname() == "hello\\world");
#endif
  REQUIRE(path.basename() == "test");
}

TEST_CASE("concatenate paths", M) {
  const Path a("hello");
  const Path b("world");

  Path c;
  c.append("hello");
  c.append("world");

  REQUIRE(a + b == c);
  REQUIRE(a + "world" == c);
}

TEST_CASE("empty components", M) {
  Path a;
  a.append(string());
  a.prepend(string());

  REQUIRE(a.size() == 0);
}

TEST_CASE("clear path", M) {
  Path a;
  a.append("test");

  CHECK(a.size() == 1);
  a.clear();
  REQUIRE(a.size() == 0);
}

TEST_CASE("modify path", M) {
  Path a;
  a.append("hello");

  a[0] = "world";
  REQUIRE(a.join() == "world");
}

TEST_CASE("custom separator", M) {
  Path a;
  a.append("hello");
  a.append("world");

  REQUIRE(a.join('-') == "hello-world");
}

TEST_CASE("split input", M) {
  SECTION("slash") {
    const Path a("hello/world");

    REQUIRE(a.size() == 2);
    REQUIRE(a[0] == "hello");
    REQUIRE(a[1] == "world");
  }

  SECTION("backslash") {
    const Path a("hello\\world");

    REQUIRE(a.size() == 2);
    REQUIRE(a[0] == "hello");
    REQUIRE(a[1] == "world");
  }

  SECTION("prepend") {
    Path a;
    a.prepend("hello/world");

    REQUIRE(a.size() == 2);
  }

  SECTION("append") {
    Path a;
    a.append("hello/world");

    REQUIRE(a.size() == 2);
  }

  SECTION("skip empty parts") {
    const Path a("hello//world/");

    REQUIRE(a.size() == 2);
    REQUIRE(a[0] == "hello");
    REQUIRE(a[1] == "world");
  }
}

#ifndef _WIN32
TEST_CASE("absolute path (unix)", M) {
  const Path a("/usr/bin/zsh");

  REQUIRE(a.size() == 3);
  REQUIRE(a[0] == "/usr");
  REQUIRE(a.join() == "/usr/bin/zsh");
}
#endif

TEST_CASE("remove last component of path", M) {
  Path a;
  a.append("a");
  a.append("b");

  CHECK(a.size() == 2);

  a.removeLast();

  REQUIRE(a.size() == 1);
  REQUIRE(a[0] == "a");
}

TEST_CASE("path generation utilities", M) {
  const Path path("world");

  REQUIRE(Path::prefixRoot(path) == Path("world"));
  REQUIRE(Path::cacheDir() == Path("ReaPack"));
  REQUIRE(Path::prefixCache("test") == Path("ReaPack/test"));

  {
    UseRootPath root("hello");
    (void)root;

    REQUIRE(Path::prefixRoot(path) == Path("hello/world"));
    REQUIRE(Path::prefixRoot("world") == Path("hello/world"));

    REQUIRE(Path::cacheDir() == Path("hello/ReaPack"));
    REQUIRE(Path::prefixCache("test") == Path("hello/ReaPack/test"));
  }

  REQUIRE(Path::prefixRoot(path) == Path("world"));
}
