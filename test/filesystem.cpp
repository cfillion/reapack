#include "helper.hpp"

#include <filesystem.hpp>
#include <index.hpp>

static const char *M = "[filesystem]";
static const Path RIPATH(L("test/indexes"));
static const char *CYRILLIC = "Новая папка";

TEST_CASE("open unicode file", M) {
  UseRootPath root(RIPATH);
  const Path &path = Index::pathFor(CYRILLIC);

  FILE *file = FS::open(path);
  REQUIRE(file);
  fclose(file);
}

TEST_CASE("file modification time", M) {
  UseRootPath root(RIPATH);
  const Path &path = Index::pathFor(CYRILLIC);

  time_t time = 0;
  REQUIRE(FS::mtime(path, &time));
  REQUIRE(time > 0);
}

TEST_CASE("file exists", M) {
  UseRootPath root(RIPATH);

  REQUIRE(FS::exists(Index::pathFor(CYRILLIC)));
  REQUIRE_FALSE(FS::exists(Index::pathFor(CYRILLIC), true));

  REQUIRE_FALSE(FS::exists(Path(L("ReaPack"))));
  REQUIRE(FS::exists(Path(L("ReaPack")), true));
}
