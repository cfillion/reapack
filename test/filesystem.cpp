#include <catch.hpp>

#include <filesystem.hpp>
#include <index.hpp>

static const char *M = "[filesystem]";

#define RIPATH "test/indexes"

TEST_CASE("unicode filename", M) {
  UseRootPath root(RIPATH);
  const Path &path = Index::pathFor("Новая папка");

  SECTION("FS::open") {
    FILE *file = FS::open(path);
    REQUIRE(file);
    fclose(file);
  }

  SECTION("FS::mtime") {
    time_t time;
    REQUIRE(FS::mtime(path, &time));
  }
}
