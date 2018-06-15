#include "helper.hpp"

#include <errors.hpp>
#include <index.hpp>
#include <remote.hpp>

using namespace std;

static const char *M = "[reapack_v1]";
static const Path RIPATH("test/indexes/v1");

TEST_CASE("unnamed category", M) {
  UseRootPath root(RIPATH);

  try {
    RemotePtr re = make_shared<Remote>("unnamed_category", "url");
  Index ri(re);
    ri.load();
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "empty category name");
  }
}

TEST_CASE("invalid category tag", M) {
  UseRootPath root(RIPATH);

  RemotePtr re = make_shared<Remote>("wrong_category_tag", "url");
  Index ri(re);
  ri.load();

  REQUIRE(ri.categories().empty());
}

TEST_CASE("invalid package tag", M) {
  UseRootPath root(RIPATH);

  RemotePtr re = make_shared<Remote>("wrong_package_tag", "url");
  Index ri(re);
  ri.load();
  REQUIRE(ri.categories().empty());

}

TEST_CASE("null package name", M) {
  UseRootPath root(RIPATH);

  try {
    RemotePtr re = make_shared<Remote>("unnamed_package", "url");
  Index ri(re);
    ri.load();
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "empty package name");
  }
}

TEST_CASE("null package type", M) {
  UseRootPath root(RIPATH);

  try {
    RemotePtr re = make_shared<Remote>("missing_type", "url");
  Index ri(re);
    ri.load();
  }
  catch(const reapack_error &) {
    // no segfault -> test passes!
  }
}

TEST_CASE("unsupported package type", M) {
  UseRootPath root(RIPATH);

  RemotePtr re = make_shared<Remote>("unsupported_type", "url");
  Index ri(re);
    ri.load();
}

TEST_CASE("read version author", M) {
  UseRootPath root(RIPATH);

  RemotePtr re = make_shared<Remote>("author", "url");
  Index ri(re);
  ri.load();

  CHECK(ri.packages().size() == 1);
  REQUIRE(ri.category(0)->package(0)->version(0)->author()
    == "Watanabe Saki");
}

TEST_CASE("read version time", M) {
  UseRootPath root(RIPATH);

  RemotePtr re = make_shared<Remote>("time", "url");
  Index ri(re);
  ri.load();
  CHECK(ri.packages().size() == 1);

  const Time &time = ri.category(0)->package(0)->version(0)->time();
  REQUIRE(time.year() == 2016);
  REQUIRE(time.month() == 2);
  REQUIRE(time.day() == 12);
}

TEST_CASE("invalid version tag", M) {
  UseRootPath root(RIPATH);

  RemotePtr re = make_shared<Remote>("wrong_version_tag", "url");
  Index ri(re);
    ri.load();
  REQUIRE(ri.categories().empty());
}

TEST_CASE("null package version", M) {
  UseRootPath root(RIPATH);

  try {
    RemotePtr re = make_shared<Remote>("missing_version", "url");
  Index ri(re);
    ri.load();
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "invalid version name ''");
  }
}

TEST_CASE("null source url", M) {
  UseRootPath root(RIPATH);

  try {
    RemotePtr re = make_shared<Remote>("missing_source_url", "url");
  Index ri(re);
    ri.load();
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "empty source url");
  }
}

TEST_CASE("null source file", M) {
  UseRootPath root(RIPATH);

  RemotePtr re = make_shared<Remote>("missing_source_file", "url");
  Index ri(re);
  ri.load();
  CHECK(ri.packages().size() == 1);

  const Package *pkg = ri.category(0)->package(0);
  REQUIRE(pkg->version(0)->source(0)->file() == pkg->name());
}

TEST_CASE("default source platform", M) {
  UseRootPath root(RIPATH);

  RemotePtr re = make_shared<Remote>("missing_platform", "url");
  Index ri(re);
  ri.load();

  CHECK(ri.packages().size() == 1);
  REQUIRE(ri.category(0)->package(0)->version(0)->source(0)->platform()
    == Platform::GenericPlatform);
}

TEST_CASE("version changelog", M) {
  UseRootPath root(RIPATH);

  RemotePtr re = make_shared<Remote>("changelog", "url");
  Index ri(re);
  ri.load();
  CHECK(ri.packages().size() == 1);

  REQUIRE(ri.category(0)->package(0)->version(0)->changelog()
    == "Hello\nWorld");
}

TEST_CASE("full index", M) {
  UseRootPath root(RIPATH);

  RemotePtr re = make_shared<Remote>("valid_index", "url");
  Index ri(re);
  ri.load();

  REQUIRE(ri.categories().size() == 1);
  const Category *cat = ri.category(0);
  REQUIRE(cat->name() == "Category Name");

  REQUIRE(cat->packages().size() == 1);
  const Package *pack = cat->package(0);
  REQUIRE(pack->type() == Package::ScriptType);
  REQUIRE(pack->name() == "Hello World.lua");

  REQUIRE(pack->versions().size() == 1);
  const Version *ver = pack->version(0);
  REQUIRE(ver->name() == VersionName("1.0"));
  REQUIRE(ver->changelog() == "Fixed a division by zero error.");

  REQUIRE(ver->sources().size() == 2);
  const Source *source1 = ver->source(0);
  REQUIRE(source1->platform() == Platform::GenericPlatform);
  REQUIRE(source1->file() == "test.lua");
  REQUIRE(source1->sections() == Source::MainSection);
  REQUIRE(source1->url() == "https://google.com/");

  const Source *source2 = ver->source(1);
  REQUIRE(source2->platform() == Platform::GenericPlatform);
  REQUIRE(source2->file() == "background.png");
  REQUIRE_FALSE(source2->sections() == Source::MainSection);
  REQUIRE(source2->url() == "http://cfillion.tk/");
}

TEST_CASE("read index metadata", M) {
  UseRootPath root(RIPATH);

  RemotePtr re = make_shared<Remote>("metadata", "url");
  Index ri(re);
  ri.load();

  SECTION("description") {
    REQUIRE(ri.metadata()->about() == "Chunky\nBacon");
  }

  SECTION("links") {
    REQUIRE(ri.metadata()->links().size() == 5);

    auto link = ri.metadata()->links().begin();
    REQUIRE(link->first == Metadata::WebsiteLink);
    REQUIRE(link->second.name == "http://cfillion.tk");
    REQUIRE(link->second.url == "http://cfillion.tk");

    REQUIRE((++link)->second.name == "https://github.com/cfillion");
    REQUIRE(link->second.url == "https://github.com/cfillion");

    REQUIRE((++link)->second.name == "http://twitter.com/cfi30");
    REQUIRE(link->second.url == "http://twitter.com/cfi30");

    REQUIRE((++link)->second.name == "http://google.com");
    REQUIRE(link->second.url == "http://google.com");

    REQUIRE((++link)->first == Metadata::DonationLink);
    REQUIRE(link->second.name == "Donate");
    REQUIRE(link->second.url == "http://paypal.com");
  }
}

TEST_CASE("read package metadata", M) {
  UseRootPath root(RIPATH);

  RemotePtr re = make_shared<Remote>("pkg_metadata", "url");
  Index ri(re);
  ri.load();
  REQUIRE(ri.packages().size() == 1);

  const Package *pkg = ri.packages()[0];

  SECTION("description") {
    REQUIRE(pkg->metadata()->about() == "Chunky\nBacon");
  }

  SECTION("links") {
    REQUIRE(pkg->metadata()->links().size() == 2);

    auto link = pkg->metadata()->links().begin();
    REQUIRE(link->first == Metadata::WebsiteLink);
    REQUIRE(link->second.name == "http://cfillion.tk");
    REQUIRE(link->second.url == "http://cfillion.tk");

    REQUIRE((++link)->first == Metadata::DonationLink);
    REQUIRE(link->second.name == "Donate");
    REQUIRE(link->second.url == "http://paypal.com");
  }
}

TEST_CASE("read index name (from raw data only)") {
  Index ri;

  SECTION("valid") {
    const string &name = ri.load("<index version=\"1\" name=\"Hello World\"/>\n");
    REQUIRE(name == "Hello World");
  }

  SECTION("missing") {
    const string &name = ri.load("<index version=\"1\"/>\n");
    REQUIRE(name == "");
  }
}

TEST_CASE("read source platform", M) {
  UseRootPath root(RIPATH);

  RemotePtr re = make_shared<Remote>("src_platform", "url");
  Index ri(re);
  ri.load();

  REQUIRE(ri.packages().size() == 1);

#ifdef __APPLE__
  const auto expected = Platform::DarwinPlatform;
#elif _WIN32
  const auto expected = Platform::WindowsPlatform;
#elif __linux__
  const auto expected = Platform::LinuxPlatform;
#endif

  REQUIRE(ri.category(0)->package(0)->version(0)->source(0)->platform()
    == expected);
}

TEST_CASE("read source type override", M) {
  UseRootPath root(RIPATH);

  RemotePtr re = make_shared<Remote>("src_type", "url");
  Index ri(re);
  ri.load();

  CHECK(ri.packages().size() == 1);
  REQUIRE(ri.category(0)->package(0)->version(0)->source(0)->typeOverride()
    == Package::EffectType);
}

TEST_CASE("read package description", M) {
  UseRootPath root(RIPATH);

  RemotePtr re = make_shared<Remote>("pkg_desc", "url");
  Index ri(re);
  ri.load();

  CHECK(ri.packages().size() == 1);
  REQUIRE(ri.category(0)->package(0)->description() == "From the New World");
}

TEST_CASE("read multiple sections", M) {
  UseRootPath root(RIPATH);

  RemotePtr re = make_shared<Remote>("explicit_sections", "url");
  Index ri(re);
  ri.load();

  CHECK(ri.packages().size() == 1);
  REQUIRE(ri.category(0)->package(0)->version(0)->source(0)->sections()
    == (Source::MainSection | Source::MIDIEditorSection));
}
