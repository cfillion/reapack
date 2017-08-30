#include "helper.hpp"

#include <index.hpp>
#include <errors.hpp>

#include <string>

using namespace std;

static const char *M = "[reapack_v1]";
static const Path RIPATH(L"test/indexes");

TEST_CASE("unnamed category", M) {
  UseRootPath root(RIPATH);

  try {
    IndexPtr ri = Index::load("unnamed_category");
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(e.what() == L"empty category name");
  }
}

TEST_CASE("invalid category tag", M) {
  UseRootPath root(RIPATH);

  IndexPtr ri = Index::load("wrong_category_tag");

  REQUIRE(ri->categories().empty());
}

TEST_CASE("invalid package tag", M) {
  UseRootPath root(RIPATH);

  IndexPtr ri = Index::load("wrong_package_tag");
  REQUIRE(ri->categories().empty());

}

TEST_CASE("null package name", M) {
  UseRootPath root(RIPATH);

  try {
    IndexPtr ri = Index::load("unnamed_package");
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(e.what() == L"empty package name");
  }
}

TEST_CASE("null package type", M) {
  UseRootPath root(RIPATH);

  try {
    IndexPtr ri = Index::load("missing_type");
  }
  catch(const reapack_error &) {
    // no segfault -> test passes!
  }
}

TEST_CASE("unsupported package type", M) {
  UseRootPath root(RIPATH);

  IndexPtr ri = Index::load("unsupported_type");
}

TEST_CASE("read version author", M) {
  UseRootPath root(RIPATH);

  IndexPtr ri = Index::load("author");

  CHECK(ri->packages().size() == 1);
  REQUIRE(ri->category(0)->package(0)->version(0)->author()
    == L"Watanabe Saki");
}

TEST_CASE("read version time", M) {
  UseRootPath root(RIPATH);

  IndexPtr ri = Index::load("time");
  CHECK(ri->packages().size() == 1);

  const Time &time = ri->category(0)->package(0)->version(0)->time();
  REQUIRE(time.year() == 2016);
  REQUIRE(time.month() == 2);
  REQUIRE(time.day() == 12);
}

TEST_CASE("invalid version tag", M) {
  UseRootPath root(RIPATH);

  IndexPtr ri = Index::load("wrong_version_tag");
  REQUIRE(ri->categories().empty());
}

TEST_CASE("null package version", M) {
  UseRootPath root(RIPATH);

  try {
    IndexPtr ri = Index::load("missing_version");
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(e.what() == L"invalid version name ''");
  }
}

TEST_CASE("null source url", M) {
  UseRootPath root(RIPATH);

  try {
    IndexPtr ri = Index::load("missing_source_url");
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(e.what() == L"empty source url");
  }
}

TEST_CASE("null source file", M) {
  UseRootPath root(RIPATH);

  IndexPtr ri = Index::load("missing_source_file");
  CHECK(ri->packages().size() == 1);

  const Package *pkg = ri->category(0)->package(0);
  REQUIRE(pkg->version(0)->source(0)->file() == pkg->name());
}

TEST_CASE("default source platform", M) {
  UseRootPath root(RIPATH);

  IndexPtr ri = Index::load("missing_platform");

  CHECK(ri->packages().size() == 1);
  REQUIRE(ri->category(0)->package(0)->version(0)->source(0)->platform()
    == Platform::GenericPlatform);
}

TEST_CASE("version changelog", M) {
  UseRootPath root(RIPATH);

  IndexPtr ri = Index::load("changelog");
  CHECK(ri->packages().size() == 1);

  REQUIRE(ri->category(0)->package(0)->version(0)->changelog()
    == L"Hello\nWorld");
}

TEST_CASE("full index", M) {
  UseRootPath root(RIPATH);

  IndexPtr ri = Index::load("valid_index");

  REQUIRE(ri->categories().size() == 1);
  const Category *cat = ri->category(0);
  REQUIRE(cat->name() == L"Category Name");

  REQUIRE(cat->packages().size() == 1);
  const Package *pack = cat->package(0);
  REQUIRE(pack->type() == Package::ScriptType);
  REQUIRE(pack->name() == L"Hello World.lua");

  REQUIRE(pack->versions().size() == 1);
  const Version *ver = pack->version(0);
  REQUIRE(ver->name() == VersionName("1.0"));
  REQUIRE(ver->changelog() == L"Fixed a division by zero error.");

  REQUIRE(ver->sources().size() == 2);
  const Source *source1 = ver->source(0);
  REQUIRE(source1->platform() == Platform::GenericPlatform);
  REQUIRE(source1->file() == L"test.lua");
  REQUIRE(source1->sections() == Source::MainSection);
  REQUIRE(source1->url() == L"https://google.com/");

  const Source *source2 = ver->source(1);
  REQUIRE(source2->platform() == Platform::GenericPlatform);
  REQUIRE(source2->file() == L"background.png");
  REQUIRE_FALSE(source2->sections() == Source::MainSection);
  REQUIRE(source2->url() == L"http://cfillion.tk/");
}

TEST_CASE("read index metadata", M) {
  UseRootPath root(RIPATH);

  IndexPtr ri = Index::load("metadata");

  SECTION("name (ignored)") {
    REQUIRE(ri->name() == L"metadata");
  }

  SECTION("description") {
    REQUIRE(ri->metadata()->about() == L"Chunky\nBacon");
  }

  SECTION("links") {
    REQUIRE(ri->metadata()->links().size() == 5);

    auto link = ri->metadata()->links().begin();
    REQUIRE(link->first == Metadata::WebsiteLink);
    REQUIRE(link->second.name == L"http://cfillion.tk");
    REQUIRE(link->second.url == L"http://cfillion.tk");

    REQUIRE((++link)->second.name == L"https://github.com/cfillion");
    REQUIRE(link->second.url == L"https://github.com/cfillion");

    REQUIRE((++link)->second.name == L"http://twitter.com/cfi30");
    REQUIRE(link->second.url == L"http://twitter.com/cfi30");

    REQUIRE((++link)->second.name == L"http://google.com");
    REQUIRE(link->second.url == L"http://google.com");

    REQUIRE((++link)->first == Metadata::DonationLink);
    REQUIRE(link->second.name == L"Donate");
    REQUIRE(link->second.url == L"http://paypal.com");
  }
}

TEST_CASE("read package metadata", M) {
  UseRootPath root(RIPATH);

  IndexPtr ri = Index::load("pkg_metadata");
  REQUIRE(ri->packages().size() == 1);

  const Package *pkg = ri->packages()[0];

  SECTION("description") {
    REQUIRE(pkg->metadata()->about() == L"Chunky\nBacon");
  }

  SECTION("links") {
    REQUIRE(pkg->metadata()->links().size() == 2);

    auto link = pkg->metadata()->links().begin();
    REQUIRE(link->first == Metadata::WebsiteLink);
    REQUIRE(link->second.name == L"http://cfillion.tk");
    REQUIRE(link->second.url == L"http://cfillion.tk");

    REQUIRE((++link)->first == Metadata::DonationLink);
    REQUIRE(link->second.name == L"Donate");
    REQUIRE(link->second.url == L"http://paypal.com");
  }
}

TEST_CASE("read index name (from raw data only)") {
  SECTION("valid") {
    IndexPtr ri = Index::load({}, "<index version=\"1\" name=\"Hello World\"/>\n");
    REQUIRE(ri->name() == L"Hello World");
  }

  SECTION("missing") {
    IndexPtr ri = Index::load({}, "<index version=\"1\"/>\n");
    REQUIRE(ri->name() == L"");
  }
}

TEST_CASE("read source platform", M) {
  UseRootPath root(RIPATH);

  IndexPtr ri = Index::load("src_platform");

  REQUIRE(ri->packages().size() == 1);

#ifdef __APPLE__
  const auto expected = Platform::DarwinPlatform;
#elif _WIN32
  const auto expected = Platform::WindowsPlatform;
#elif __linux__
  const auto expected = Platform::LinuxPlatform;
#endif

  REQUIRE(ri->category(0)->package(0)->version(0)->source(0)->platform()
    == expected);
}

TEST_CASE("read source type override", M) {
  UseRootPath root(RIPATH);

  IndexPtr ri = Index::load("src_type");

  CHECK(ri->packages().size() == 1);
  REQUIRE(ri->category(0)->package(0)->version(0)->source(0)->typeOverride()
    == Package::EffectType);
}

TEST_CASE("read package description", M) {
  UseRootPath root(RIPATH);

  IndexPtr ri = Index::load("pkg_desc");

  CHECK(ri->packages().size() == 1);
  REQUIRE(ri->category(0)->package(0)->description() == L"From the New World");
}

TEST_CASE("read multiple sections", M) {
  UseRootPath root(RIPATH);

  IndexPtr ri = Index::load("explicit_sections");

  CHECK(ri->packages().size() == 1);
  REQUIRE(ri->category(0)->package(0)->version(0)->source(0)->sections()
    == (Source::MainSection | Source::MIDIEditorSection));
}
