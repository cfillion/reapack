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

TEST_CASE("append path components", M) {
  Path path;
  REQUIRE(path.empty());
  REQUIRE(path.size() == 0);
  REQUIRE(path.join() == string());
  REQUIRE(path.dirname().empty());
  REQUIRE(path.basename().empty());

  path.append("hello");
  REQUIRE_FALSE(path.empty());
  REQUIRE(path.size() == 1);
  REQUIRE(path.join() == "hello");
  REQUIRE(path.dirname().empty());
  REQUIRE(path.basename() == "hello");

  path.append("world");
  REQUIRE(path.size() == 2);
#ifndef _WIN32
  REQUIRE(path.join() == "hello/world");
#else
  REQUIRE(path.join() == "hello\\world");
#endif
  REQUIRE(path.dirname().join() == "hello");
  REQUIRE(path.basename() == "world");

  path.append("test");
  REQUIRE(path.size() == 3);
#ifndef _WIN32
  REQUIRE(path.join() == "hello/world/test");
#else
  REQUIRE(path.join() == "hello\\world\\test");
#endif
  REQUIRE(path.dirname() == Path("hello/world"));
  REQUIRE(path.basename() == "test");
}

TEST_CASE("concatenate paths", M) {
  const Path a("hello");
  const Path b("world");

  Path c;
  c.append("hello");
  c += "world";

  REQUIRE(a + b == c);
  REQUIRE(a + "world" == c);
}

TEST_CASE("empty components", M) {
  Path a;
  a.append(string());

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

  SECTION("append") {
    Path a;
    a.append("hello/world");
    REQUIRE(a.size() == 2);

    a += "chunky/bacon";
    REQUIRE(a.size() == 4);
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

  a.removeLast();
  REQUIRE(a.empty());

  a.removeLast(); // no crash
}

TEST_CASE("path generation utilities", M) {
  const Path path("world");

  REQUIRE(Path::prefixRoot(path) == Path("world"));

  {
    UseRootPath root("hello");
    (void)root;

    REQUIRE(Path::prefixRoot(path) == Path("hello/world"));
    REQUIRE(Path::prefixRoot("world") == Path("hello/world"));
  }

  REQUIRE(Path::prefixRoot(path) == Path("world"));
}

TEST_CASE("first and last path component", M) {
  REQUIRE(Path().first().empty());
  REQUIRE(Path().last().empty());

  const Path path("hello/world/chunky/bacon");
  REQUIRE(path.first() == "hello");
  REQUIRE(path.last() == "bacon");
}

TEST_CASE("directory traversal", M) {
  SECTION("append (enabled)") {
    REQUIRE(Path("a/./b") == Path("a/b"));
    REQUIRE(Path("a/../b") == Path("b"));
    REQUIRE(Path("../a") == Path("a"));
  }

  SECTION("append (disabled)") {
    Path a;
    a.append("a/../b", false);
    REQUIRE(a == Path("a/b"));
  }

  SECTION("concatenate") {
    // concatenating a std::string to a path, directory traversal is applied
    const Path a = Path("a/b/c") + "../../../d/e/f";
    REQUIRE(a == Path("d/e/f"));

    // here, the directory traversal components are lost before concatenating
    const Path b = Path("a/b/c") + Path("../../../d/e/f");
    REQUIRE(b == Path("a/b/c/d/e/f"));
  }
}

TEST_CASE("append full paths") {
  Path a;
  a += Path("a/b");
  a.append(Path("c/d"));
  a.append(Path("e/f"));

  REQUIRE(a == Path("a/b/c/d/e/f"));
}
