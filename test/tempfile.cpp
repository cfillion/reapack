#include "helper.hpp"

#include <tempfile.hpp>

static const char *M = "[tempfile]";

TEST_CASE("temporary file", M) {
  TempFile a(Path("hello/world"));

  REQUIRE(a.targetPath() == Path("hello/world"));
  REQUIRE(a.tempPath() == Path("hello/world.part"));
}
