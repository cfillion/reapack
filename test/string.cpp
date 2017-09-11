#include "helper.hpp"

#include <string.hpp>

static constexpr const char *M = "[string]";

using namespace std;

TEST_CASE("string format", M) {
  const string &formatted = String::format("%d%% Hello %s!", 100, "World");
  CHECK(formatted.size() == 17);
  REQUIRE(formatted == "100% Hello World!");
}
