#include "helper.hpp"

#include <filesystem.hpp>
#include <index.hpp>

static const char *M = "[filesystem]";
static const Path RIPATH("test/indexes");

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

  SECTION("file") {
    const Path &file = Index::pathFor("Новая папка");
    REQUIRE(FS::exists(file));
    REQUIRE_FALSE(FS::exists(file, true));
  }

  SECTION("directory") {
    REQUIRE_FALSE(FS::exists(Path("ReaPack")));
    REQUIRE(FS::exists(Path("ReaPack"), true));
  }
}

TEST_CASE("all files exists", M) {
  UseRootPath root(RIPATH);

  REQUIRE(FS::allFilesExists({}));

  REQUIRE(FS::allFilesExists({
    Index::pathFor("future_version"),
    Index::pathFor("broken"),
  }));

  REQUIRE_FALSE(FS::allFilesExists({
    Index::pathFor("future_version"),
    Index::pathFor("not_found"),
  }));

  REQUIRE_FALSE(FS::allFilesExists({Path("ReaPack")})); // directory
}
