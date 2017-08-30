#include "helper.hpp"

#include <index.hpp>
#include <source.hpp>
#include <version.hpp>

#include <errors.hpp>

using namespace std;

static const char *M = "[source]";

#define MAKE_VERSION \
  Index ri(L("Index Name")); \
  Category cat(L("Category Name"), &ri); \
  Package pkg(Package::DataType, L("Package Name"), &cat); \
  Version ver(L("1.0"), &pkg);

TEST_CASE("source platform", M) {
  MAKE_VERSION;

  Source src({}, L("url"), &ver);
  REQUIRE(src.platform() == Platform::GenericPlatform);

  src.setPlatform(Platform::UnknownPlatform);
  REQUIRE(src.platform() == Platform::UnknownPlatform);

  src.setPlatform(Platform::WindowsPlatform);
  REQUIRE(src.platform() == Platform::WindowsPlatform);
}

TEST_CASE("source type override", M) {
  MAKE_VERSION;

  Source src({}, L("url"), &ver);
  REQUIRE(src.type() == Package::DataType);
  REQUIRE(src.typeOverride() == Package::UnknownType);

  src.setTypeOverride(Package::ScriptType);
  REQUIRE(src.type() == Package::ScriptType);
  REQUIRE(src.typeOverride() == Package::ScriptType);
}

TEST_CASE("source type from package", M) {
  MAKE_VERSION;
  Source src({}, L("url"), &ver);

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
    Source source(L("filename"), L("url"), &ver);
    REQUIRE(source.sections() == 0);

    source.setTypeOverride(Package::ScriptType);
    CHECK(source.type() == Package::ScriptType);

    source.setSections(Source::MainSection | Source::MIDIEditorSection);
    REQUIRE(source.sections() == (Source::MainSection | Source::MIDIEditorSection));
  }

  SECTION("other type") {
    Source source(L("filename"), L("url"), &ver);
    source.setSections(Source::MainSection);
    CHECK(source.type() == Package::DataType);
    REQUIRE(source.sections() == 0);
  }
}

TEST_CASE("implicit source section") {
  Index ri(L("Index Name"));

  SECTION("main") {
    Category cat(L("Category Name"), &ri);
    Package pack(Package::ScriptType, L("package name"), &cat);
    Version ver(L("1.0"), &pack);

    Source source(L("filename"), L("url"), &ver);
    source.setSections(Source::ImplicitSection);
    REQUIRE(source.sections() == Source::MainSection);
  }

  SECTION("midi editor") {
    Category cat(L("MIDI Editor"), &ri);
    Package pack(Package::ScriptType, L("package name"), &cat);
    Version ver(L("1.0"), &pack);

    Source source(L("filename"), L("url"), &ver);
    source.setSections(Source::ImplicitSection);
    REQUIRE(source.sections() == Source::MIDIEditorSection);
  }

  SECTION("midi inline editor") {
    Category cat(L("MIDI Inline Editor"), &ri);
    Package pack(Package::ScriptType, L("package name"), &cat);
    Version ver(L("1.0"), &pack);

    Source source(L("filename"), L("url"), &ver);
    source.setSections(Source::ImplicitSection);
    REQUIRE(source.sections() == Source::MIDIInlineEditorSection);
  }
}

TEST_CASE("implicit section detection", M) {
  REQUIRE(Source::MainSection == Source::detectSection(L("Hello World")));
  REQUIRE(Source::MainSection == Source::detectSection(L("Hello/World")));
  REQUIRE(Source::MainSection == Source::detectSection(L("Hello/midi editor")));

  REQUIRE(Source::MIDIEditorSection == Source::detectSection(L("midi editor")));
  REQUIRE(Source::MIDIEditorSection == Source::detectSection(L("midi editor/Hello")));

  REQUIRE(Source::MIDIInlineEditorSection ==
    Source::detectSection(L("midi inline editor")));
}

TEST_CASE("empty source url", M) {
  MAKE_VERSION;

  try {
    const Source source(L("filename"), {}, &ver);
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(e.what() == L("empty source url"));
  }
}

TEST_CASE("source target path", M) {
  MAKE_VERSION;

  Source source(L("file.name"), L("url"), &ver);

  const vector<pair<Package::Type, String> > tests{
    {Package::ScriptType,          L("Scripts/Index Name/Category Name/file.name")},
    {Package::EffectType,          L("Effects/Index Name/Category Name/file.name")},
    {Package::ExtensionType,       L("UserPlugins/file.name")},
    {Package::DataType,            L("Data/file.name")},
    {Package::ThemeType,           L("ColorThemes/file.name")},
    {Package::LangPackType,        L("LangPack/file.name")},
    {Package::WebInterfaceType,    L("reaper_www_root/file.name")},
    {Package::ProjectTemplateType, L("ProjectTemplates/file.name")},
    {Package::TrackTemplateType,   L("TrackTemplates/file.name")},
    {Package::MIDINoteNamesType,   L("MIDINoteNames/file.name")},
  };

  for(const auto &pair : tests) {
    source.setTypeOverride(pair.first);
    REQUIRE(source.targetPath() == Path(pair.second));
  }
}

TEST_CASE("target path with parent directory traversal", M) {
  Index ri(L("Index Name"));
  Category cat(L("Category Name"), &ri);
  Package pack(Package::ScriptType, L("package name"), &cat);
  Version ver(L("1.0"), &pack);
  Source source(L("../../../file.name"), L("url"), &ver);

  SECTION("script") {
    Path expected;
    expected.append(L("Scripts"));
    expected.append(L("Index Name"));
    // expected.append(L("Category Name")); // only the category can be bypassed!
    expected.append(L("file.name"));

    REQUIRE(source.targetPath() == expected);
  }

  SECTION("extension") {
    Path expected;
    expected.append(L("UserPlugins"));
    expected.append(L("file.name"));
    source.setTypeOverride(Package::ExtensionType);
    REQUIRE(source.targetPath() == expected);
  }
}

TEST_CASE("target path for unknown package type", M) {
  Index ri(L("name"));
  Category cat(L("name"), &ri);
  Package pack(Package::UnknownType, L("a"), &cat);
  Version ver(L("1.0"), &pack);
  Source src({}, L("url"), &ver);

  REQUIRE(src.targetPath().empty());
}

TEST_CASE("directory traversal in category name", M) {
  Index ri(L("Remote Name"));
  Category cat(L("../.."), &ri);
  Package pack(Package::ScriptType, L("file.name"), &cat);
  Version ver(L("1.0"), &pack);
  Source src({}, L("url"), &ver);

  Path expected;
  expected.append(L("Scripts"));
  expected.append(L("Remote Name"));
  expected.append(L("file.name"));

  REQUIRE(src.targetPath() == expected);
}
