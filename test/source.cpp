#include <catch.hpp>

#include <index.hpp>
#include <source.hpp>
#include <version.hpp>

#include <errors.hpp>

using namespace std;

static const char *M = "[source]";

#define MAKE_VERSION \
  Index ri("Index Name"); \
  Category cat("Category Name", &ri); \
  Package pkg(Package::DataType, "Package Name", &cat); \
  Version ver("1.0", &pkg);

TEST_CASE("source platform", M) {
  MAKE_VERSION;

  Source src({}, "url", &ver);
  REQUIRE(src.platform() == Platform::GenericPlatform);

  src.setPlatform(Platform::UnknownPlatform);
  REQUIRE(src.platform() == Platform::UnknownPlatform);

  src.setPlatform(Platform::WindowsPlatform);
  REQUIRE(src.platform() == Platform::WindowsPlatform);
}

TEST_CASE("source type override", M) {
  MAKE_VERSION;

  Source src({}, "url", &ver);
  REQUIRE(src.type() == Package::DataType);
  REQUIRE(src.typeOverride() == Package::UnknownType);

  src.setTypeOverride(Package::ScriptType);
  REQUIRE(src.type() == Package::ScriptType);
  REQUIRE(src.typeOverride() == Package::ScriptType);
}

TEST_CASE("source type from package", M) {
  MAKE_VERSION;
  Source src({}, "url", &ver);

  REQUIRE(src.version() == &ver);

  REQUIRE(src.type() == Package::DataType);

  src.setTypeOverride(Package::ScriptType);
  REQUIRE(src.type() == Package::ScriptType);
}

TEST_CASE("parse file section", M) {
  REQUIRE(-1 == Source::getSection("true"));
  REQUIRE(0 == Source::getSection("hello"));
  REQUIRE(Source::MainSection == Source::getSection("main"));
  REQUIRE(Source::MIDIEditorSection == Source::getSection("midi_editor"));
  REQUIRE(Source::MIDIInlineEditorSection == Source::getSection("midi_inline_editor"));
}

TEST_CASE("explicit source section", M) {
  MAKE_VERSION;

  SECTION("script type") {
    Source source("filename", "url", &ver);
    REQUIRE(source.sections() == 0);

    source.setTypeOverride(Package::ScriptType);
    CHECK(source.type() == Package::ScriptType);

    source.setSections(Source::MainSection | Source::MIDIEditorSection);
    REQUIRE(source.sections() == (Source::MainSection | Source::MIDIEditorSection));
  }

  SECTION("other type") {
    Source source("filename", "url", &ver);
    source.setSections(Source::MainSection);
    CHECK(source.type() == Package::DataType);
    REQUIRE(source.sections() == 0);
  }
}

TEST_CASE("implicit source section") {
  Index ri("Index Name");

  SECTION("main") {
    Category cat("Category Name", &ri);
    Package pack(Package::ScriptType, "package name", &cat);
    Version ver("1.0", &pack);

    Source source("filename", "url", &ver);
    source.setSections(Source::ImplicitSection);
    REQUIRE(source.sections() == Source::MainSection);
  }

  SECTION("midi editor") {
    Category cat("MIDI Editor", &ri);
    Package pack(Package::ScriptType, "package name", &cat);
    Version ver("1.0", &pack);

    Source source("filename", "url", &ver);
    source.setSections(Source::ImplicitSection);
    REQUIRE(source.sections() == Source::MIDIEditorSection);
  }

  SECTION("midi inline editor") {
    Category cat("MIDI Inline Editor", &ri);
    Package pack(Package::ScriptType, "package name", &cat);
    Version ver("1.0", &pack);

    Source source("filename", "url", &ver);
    source.setSections(Source::ImplicitSection);
    REQUIRE(source.sections() == Source::MIDIInlineEditorSection);
  }
}

TEST_CASE("implicit section detection", M) {
  REQUIRE(Source::MainSection == Source::detectSection("Hello World"));
  REQUIRE(Source::MainSection == Source::detectSection("Hello/World"));
  REQUIRE(Source::MainSection == Source::detectSection("Hello/midi editor"));

  REQUIRE(Source::MIDIEditorSection == Source::detectSection("midi editor"));
  REQUIRE(Source::MIDIEditorSection == Source::detectSection("midi editor/Hello"));

  REQUIRE(Source::MIDIInlineEditorSection ==
    Source::detectSection("midi inline editor"));
}

TEST_CASE("empty source url", M) {
  MAKE_VERSION;

  try {
    const Source source("filename", {}, &ver);
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(String(e.what()) == "empty source url");
  }
}

TEST_CASE("source target path", M) {
  MAKE_VERSION;

  Source source("file.name", "url", &ver);

  const vector<pair<Package::Type, String> > tests{
    {Package::ScriptType,          "Scripts/Index Name/Category Name/file.name"},
    {Package::EffectType,          "Effects/Index Name/Category Name/file.name"},
    {Package::ExtensionType,       "UserPlugins/file.name"},
    {Package::DataType,            "Data/file.name"},
    {Package::ThemeType,           "ColorThemes/file.name"},
    {Package::LangPackType,        "LangPack/file.name"},
    {Package::WebInterfaceType,    "reaper_www_root/file.name"},
    {Package::ProjectTemplateType, "ProjectTemplates/file.name"},
    {Package::TrackTemplateType,   "TrackTemplates/file.name"},
    {Package::MIDINoteNamesType,   "MIDINoteNames/file.name"},
  };

  for(const auto &pair : tests) {
    source.setTypeOverride(pair.first);
    REQUIRE(source.targetPath() == Path(pair.second));
  }
}

TEST_CASE("target path with parent directory traversal", M) {
  Index ri("Index Name");
  Category cat("Category Name", &ri);
  Package pack(Package::ScriptType, "package name", &cat);
  Version ver("1.0", &pack);
  Source source("../../../file.name", "url", &ver);

  SECTION("script") {
    Path expected;
    expected.append("Scripts");
    expected.append("Index Name");
    // expected.append("Category Name"); // only the category can be bypassed!
    expected.append("file.name");

    REQUIRE(source.targetPath() == expected);
  }

  SECTION("extension") {
    Path expected;
    expected.append("UserPlugins");
    expected.append("file.name");
    source.setTypeOverride(Package::ExtensionType);
    REQUIRE(source.targetPath() == expected);
  }
}

TEST_CASE("target path for unknown package type", M) {
  Index ri("name");
  Category cat("name", &ri);
  Package pack(Package::UnknownType, "a", &cat);
  Version ver("1.0", &pack);
  Source src({}, "url", &ver);

  REQUIRE(src.targetPath().empty());
}

TEST_CASE("directory traversal in category name", M) {
  Index ri("Remote Name");
  Category cat("../..", &ri);
  Package pack(Package::ScriptType, "file.name", &cat);
  Version ver("1.0", &pack);
  Source src({}, "url", &ver);

  Path expected;
  expected.append("Scripts");
  expected.append("Remote Name");
  expected.append("file.name");

  REQUIRE(src.targetPath() == expected);
}
