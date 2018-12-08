#include "helper.hpp"

#include <hash.hpp>

static const char *M = "[hash]";

TEST_CASE("sha256 hashes", M) {
  Hash hash(Hash::SHA256);

  SECTION("empty")
    REQUIRE(hash.digest() ==
      "1220e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");

  SECTION("digest twice")
    REQUIRE(hash.digest() == hash.digest());

  SECTION("single chunk") {
    hash.addData("hello world", 11);

    REQUIRE(hash.digest() ==
      "1220b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9");
  }

  SECTION("split chunks") {
    hash.addData("foo bar", 7);
    hash.addData(" bazqux", 4);

    REQUIRE(hash.digest() ==
      "1220dbd318c1c462aee872f41109a4dfd3048871a03dedd0fe0e757ced57dad6f2d7");
  }
}

TEST_CASE("invalid algorithm", M) {
  Hash hash(static_cast<Hash::Algorithm>(0));
  hash.addData("foo bar", 7);
  REQUIRE(hash.digest() == "");
}

TEST_CASE("get hash algorithm", M) {
  Hash::Algorithm algo;

  SECTION("empty string")
    REQUIRE_FALSE(Hash::getAlgorithm("", &algo));

  SECTION("only sha-256 ID")
    REQUIRE_FALSE(Hash::getAlgorithm("12", &algo));

  SECTION("unexpected size")
    REQUIRE_FALSE(Hash::getAlgorithm("1202ab", &algo));

  SECTION("seemingly good (but not actually) sha-256") {
    REQUIRE(Hash::getAlgorithm("1202abcd", &algo));
    REQUIRE(algo == Hash::SHA256);
  }
}
