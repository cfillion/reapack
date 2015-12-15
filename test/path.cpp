#include <catch.hpp>

#include "helper/io.hpp"

#include <path.hpp>

using namespace std;

static const char *M = "[path]";

TEST_CASE("compare paths", M) {
  const Path a = Path() + "hello" + "world";
  const Path b = Path() + "chunky" + "bacon";

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
  Path a;
  a.append("hello");

  Path b;
  b.append("world");

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
