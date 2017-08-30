#include "helper.hpp"

#include <path.hpp>

using namespace std;

static const char *M = "[path]";

TEST_CASE("compare paths", M) {
  const Path a = Path(L("hello"));
  const Path b = Path(L("world"));

  REQUIRE_FALSE(a == b);
  REQUIRE(a != b);

  REQUIRE(a == a);
  REQUIRE_FALSE(a != a);
}

TEST_CASE("append path components", M) {
  Path path;
  REQUIRE(path.empty());
  REQUIRE(path.size() == 0);
  REQUIRE(path.join() == String());
  REQUIRE(path.dirname().empty());
  REQUIRE(path.basename().empty());

  path.append(L("hello"));
  REQUIRE_FALSE(path.empty());
  REQUIRE(path.size() == 1);
  REQUIRE(path.join() == L("hello"));
  REQUIRE(path.dirname().empty());
  REQUIRE(path.basename() == L("hello"));

  path.append(L("world"));
  REQUIRE(path.size() == 2);
#ifndef _WIN32
  REQUIRE(path.join() == L("hello/world"));
#else
  REQUIRE(path.join() == L("hello\\world"));
#endif
  REQUIRE(path.dirname().join() == L("hello"));
  REQUIRE(path.basename() == L("world"));

  path.append(L("test"));
  REQUIRE(path.size() == 3);
#ifndef _WIN32
  REQUIRE(path.join() == L("hello/world/test"));
#else
  REQUIRE(path.join() == L("hello\\world\\test"));
#endif
  REQUIRE(path.dirname() == Path(L("hello/world")));
  REQUIRE(path.basename() == L("test"));
}

TEST_CASE("concatenate paths", M) {
  const Path a(L("hello"));
  const Path b(L("world"));

  Path c;
  c.append(L("hello"));
  c += L("world");

  REQUIRE(a + b == c);
  REQUIRE(a + L("world") == c);
}

TEST_CASE("empty components", M) {
  Path a;
  a.append(String());

  REQUIRE(a.size() == 0);
}

TEST_CASE("strip trailing/leading slashes", M) {
  Path a;
  a.append(L("a/b/"));
  a.append(L("/c/d/"));
  a.append(Path(L("/e/f/")));

  REQUIRE(a.size() == 6);
#ifndef _WIN32
  REQUIRE(a.join() == L("a/b/c/d/e/f"));
#else
  REQUIRE(a.join() == L("a\\b\\c\\d\\e\\f"));
#endif
}

TEST_CASE("clear path", M) {
  Path a;
  a.append(L("test"));

  CHECK(a.size() == 1);
  a.clear();
  REQUIRE(a.size() == 0);
}

TEST_CASE("modify path", M) {
  Path a;
  a.append(L("hello"));

  a[0] = L("world");
  REQUIRE(a.join() == L("world"));
}

TEST_CASE("custom separator", M) {
  Path a;
  a.append(L("hello"));
  a.append(L("world"));

  REQUIRE(a.join('-') == L("hello-world"));
}

TEST_CASE("split input", M) {
  SECTION("slash") {
    const Path a(L("hello/world"));

    REQUIRE(a.size() == 2);
    REQUIRE(a[0] == L("hello"));
    REQUIRE(a[1] == L("world"));
  }

  SECTION("backslash") {
    const Path a(L("hello\\world"));

    REQUIRE(a.size() == 2);
    REQUIRE(a[0] == L("hello"));
    REQUIRE(a[1] == L("world"));
  }

  SECTION("append") {
    Path a;
    a.append(L("hello/world"));
    REQUIRE(a.size() == 2);

    a += L("chunky/bacon");
    REQUIRE(a.size() == 4);
  }

  SECTION("skip empty parts") {
    const Path a(L("hello//world/"));

    REQUIRE(a.size() == 2);
    REQUIRE(a[0] == L("hello"));
    REQUIRE(a[1] == L("world"));
  }
}

#ifndef _WIN32
TEST_CASE("absolute path (unix)", M) {
  CHECK_FALSE(Path(L("a/b")).absolute());

  const Path a(L("/usr/bin/zsh"));

  REQUIRE(a.size() == 3);
  REQUIRE(a.absolute());
  REQUIRE(a[0] == L("usr"));
  REQUIRE(a.join() == L("/usr/bin/zsh"));
}
#endif

TEST_CASE("remove last component of path", M) {
  Path a;
  a.append(L("a"));
  a.append(L("b"));

  CHECK(a.size() == 2);

  a.removeLast();

  REQUIRE(a.size() == 1);
  REQUIRE(a[0] == L("a"));

  a.removeLast();
  REQUIRE(a.empty());

  a.removeLast(); // no crash
}

TEST_CASE("path generation utilities", M) {
  const Path path(L("world"));

  REQUIRE(Path::prefixRoot(path) == Path(L("world")));

  {
    UseRootPath root(Path(L("hello")));
    (void)root;

    REQUIRE(Path::prefixRoot(path) == Path(L("hello/world")));
    REQUIRE(Path::prefixRoot(L("world")) == Path(L("hello/world")));
  }

  REQUIRE(Path::prefixRoot(path) == Path(L("world")));
}

TEST_CASE("first and last path component", M) {
  REQUIRE(Path().first().empty());
  REQUIRE(Path().last().empty());

  const Path path(L("hello/world/chunky/bacon"));
  REQUIRE(path.first() == L("hello"));
  REQUIRE(path.last() == L("bacon"));
}

TEST_CASE("directory traversal", M) {
  SECTION("append (enabled)") {
    REQUIRE(Path(L("a/./b")) == Path(L("a/b")));
    REQUIRE(Path(L("a/../b")) == Path(L("b")));
    REQUIRE(Path(L("../a")) == Path(L("a")));
  }

  SECTION("append (disabled)") {
    Path a;
    a.append(L("a/../b"), false);
    REQUIRE(a == Path(L("a/b")));
  }

  SECTION("concatenate") {
    // concatenating a std::string to a path, directory traversal is applied
    const Path a = Path(L("a/b/c")) + L("../../../d/e/f");
    REQUIRE(a == Path(L("d/e/f")));

    // here, the directory traversal components are lost before concatenating
    const Path b = Path(L("a/b/c")) + Path(L("../../../d/e/f"));
    REQUIRE(b == Path(L("a/b/c/d/e/f")));
  }
}

TEST_CASE("append full paths", M) {
  Path a;
  a += Path(L("a/b"));
  a.append(Path(L("c/d")));
  a.append(Path(L("e/f")));

  REQUIRE(a == Path(L("a/b/c/d/e/f")));
}

TEST_CASE("temporary path", M) {
  TempPath a(Path(L("hello/world")));

  REQUIRE(a.target() == Path(L("hello/world")));
  REQUIRE(a.temp() == Path(L("hello/world.part")));
}

TEST_CASE("path starts with", M) {
  const Path ref(L("a/b"));

  REQUIRE(ref.startsWith(ref));
  REQUIRE(Path(L("a/b/c")).startsWith(ref));
  REQUIRE_FALSE(Path(L("/a/b/c")).startsWith(ref));
  REQUIRE_FALSE(Path(L("0/a/b/c")).startsWith(ref));
  REQUIRE_FALSE(Path(L("a")).startsWith(ref));
}

TEST_CASE("remove path segments", M) {
  Path path(L("/a/b/c/d/e"));

  SECTION("remove from start") {
    path.remove(0, 1);
    REQUIRE(path == Path(L("b/c/d/e")));
    REQUIRE_FALSE(path.absolute());
  }

  SECTION("remove from middle") {
    path.remove(1, 2);
    REQUIRE(path == Path(L("/a/d/e")));
#ifndef _WIN32
    REQUIRE(path.absolute());
#endif
  }

  SECTION("remove past the end") {
    path.remove(4, 2);
    path.remove(18, 1);
    REQUIRE(path == Path(L("/a/b/c/d")));
  }
}
