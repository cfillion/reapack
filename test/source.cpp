#include "helper.hpp"

#include <index.hpp>
#include <remote.hpp>
#include <source.hpp>
#include <version.hpp>

#include <errors.hpp>

using namespace std;

static const char *M = "[source]";

#define MAKE_VERSION \
  RemotePtr re = make_shared<Remote>("Remote Name", "url"); \
  Index ri(re); \
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
  REQUIRE(Source::getSection("true") == -1);
  REQUIRE(Source::getSection("hello") == Source::UnknownSection);
  REQUIRE(Source::getSection("main") == Source::MainSection);
  REQUIRE(Source::getSection("midi_editor") == Source::MIDIEditorSection);
  REQUIRE(Source::getSection("midi_inlineeditor") == Source::MIDIInlineEditorSection);
  REQUIRE(Source::getSection("midi_eventlisteditor") == Source::MIDIEventListEditorSection);
  REQUIRE(Source::getSection("mediaexplorer") == Source::MediaExplorerSection);
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

TEST_CASE("implicit section detection (v1.0 compatibility)", M) {
  REQUIRE(Source::MainSection == Source::detectSection(Path{"Hello World"}));
  REQUIRE(Source::MainSection == Source::detectSection(Path{"Hello/World"}));
  REQUIRE(Source::MainSection == Source::detectSection(Path{"Hello/midi editor"}));

  REQUIRE(Source::MIDIEditorSection == Source::detectSection(Path{"midi editor"}));
  REQUIRE(Source::MIDIEditorSection == Source::detectSection(Path{"midi editor/Hello"}));
}

TEST_CASE("implicit section detection from source (v1.0 compatibility)") {
  RemotePtr re = make_shared<Remote>("Remote Name", "url");
  Index ri(re);

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
}

TEST_CASE("empty source url", M) {
  MAKE_VERSION;

  try {
    const Source source("filename", {}, &ver);
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(string(e.what()) == "empty source url");
  }
}

TEST_CASE("source target path", M) {
  MAKE_VERSION;

  const pair<Package::Type, string> tests[] = {
    {Package::ScriptType,          "Scripts/Remote Name/Category Name/file.name"},
    {Package::EffectType,          "Effects/Remote Name/Category Name/file.name"},
    {Package::ExtensionType,       "UserPlugins/file.name"},
    {Package::DataType,            "Data/file.name"},
    {Package::ThemeType,           "ColorThemes/file.name"},
    {Package::LangPackType,        "LangPack/file.name"},
    {Package::WebInterfaceType,    "reaper_www_root/file.name"},
    {Package::ProjectTemplateType, "ProjectTemplates/file.name"},
    {Package::TrackTemplateType,   "TrackTemplates/file.name"},
    {Package::MIDINoteNamesType,   "MIDINoteNames/file.name"},
    {Package::AutomationItemType,  "AutomationItems/Remote Name/Category Name/file.name"},
  };

  Source source("file.name", "url", &ver);
  for(const auto &[type, path] : tests) {
    source.setTypeOverride(type);
    REQUIRE(source.targetPath() == Path(path));
  }
}

TEST_CASE("target path with parent directory traversal", M) {
  RemotePtr re = make_shared<Remote>("Remote Name", "url");
  Index ri(re);
  Category cat("Category Name", &ri);
  Package pack(Package::ScriptType, "package name", &cat);
  Version ver("1.0", &pack);
  Source source("../../../file.name", "url", &ver);

  SECTION("script") {
    Path expected;
    expected.append("Scripts");
    expected.append("Remote Name");
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
  RemotePtr re = make_shared<Remote>("name", "url");
  Index ri(re);
  Category cat("name", &ri);
  Package pack(Package::UnknownType, "a", &cat);
  Version ver("1.0", &pack);
  Source src({}, "url", &ver);

  REQUIRE(src.targetPath().empty());
}

TEST_CASE("directory traversal in category name", M) {
  RemotePtr re = make_shared<Remote>("Remote Name", "url");
  Index ri(re);
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
