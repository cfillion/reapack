#include "helper.hpp"

#include <hash.hpp>

static const char *M = "[hash]";

TEST_CASE("sha256 hashes", M) {
  Hash hash(Hash::SHA256);

  SECTION("empty") {
    REQUIRE(hash.digest() ==
      "1220e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
  }

  SECTION("single chunk") {
    hash.write("hello world", 11);

    REQUIRE(hash.digest() ==
      "1220b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9");
  }

  SECTION("split chunks") {
    hash.write("foo bar", 7);
    hash.write(" bazqux", 4);

    REQUIRE(hash.digest() ==
      "1220dbd318c1c462aee872f41109a4dfd3048871a03dedd0fe0e757ced57dad6f2d7");
  }
}
