#include <catch.hpp>

#include <filesystem.hpp>
#include <index.hpp>

static const char *M = "[filesystem]";

#define RIPATH "test/indexes"

TEST_CASE("open unicode file", M) {
  UseRootPath root(RIPATH);
  const Path &path = Index::pathFor("Новая папка");

  FILE *file = FS::open(path);
  REQUIRE(file);
  fclose(file);
}

TEST_CASE("file modification time", M) {
  UseRootPath root(RIPATH);
  const Path &path = Index::pathFor("Новая папка");

  time_t time = 0;
  REQUIRE(FS::mtime(path, &time));
  REQUIRE(time > 0);
}

TEST_CASE("file exists", M) {
  UseRootPath root(RIPATH);

  REQUIRE(FS::exists(Index::pathFor("Новая папка")));
  REQUIRE_FALSE(FS::exists(Index::pathFor("Новая папка"), true));

  REQUIRE_FALSE(FS::exists(Path("ReaPack")));
  REQUIRE(FS::exists(Path("ReaPack"), true));
}
