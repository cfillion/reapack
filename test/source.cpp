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
}

TEST_CASE("implicit section detection", M) {
  REQUIRE(Source::MainSection == Source::detectSection("Hello World"));
  REQUIRE(Source::MainSection == Source::detectSection("Hello/World"));
  REQUIRE(Source::MainSection == Source::detectSection("Hello/midi editor"));

  REQUIRE(Source::MIDIEditorSection == Source::detectSection("midi editor"));
  REQUIRE(Source::MIDIEditorSection == Source::detectSection("midi editor/Hello"));
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

TEST_CASE("source full name", M) {
  MAKE_VERSION;

  SECTION("with file name") {
    const Source source("path/to/file", "b", &ver);

    REQUIRE(source.fullName() == ver.fullName() + " (file)");
  }

  SECTION("without file name") {
    const Source source({}, "b", &ver);

    REQUIRE(source.fullName() == ver.fullName());
  }
}

TEST_CASE("source target path", M) {
  MAKE_VERSION;

  Source source("file.name", "url", &ver);

  SECTION("script") {
    source.setTypeOverride(Package::ScriptType);
    const Path expected("Scripts/Index Name/Category Name/file.name");
    REQUIRE(source.targetPath() == expected);
  }

  SECTION("effect") {
    source.setTypeOverride(Package::EffectType);
    const Path expected("Effects/Index Name/Category Name/file.name");
    REQUIRE(source.targetPath() == expected);
  }

  SECTION("extension") {
    source.setTypeOverride(Package::ExtensionType);
    const Path expected("UserPlugins/file.name");
    REQUIRE(source.targetPath() == expected);
  }

  SECTION("data") {
    source.setTypeOverride(Package::DataType);
    const Path expected("Data/file.name");
    REQUIRE(source.targetPath() == expected);
  }

  SECTION("theme") {
    source.setTypeOverride(Package::ThemeType);
    const Path expected("ColorThemes/file.name");
    REQUIRE(source.targetPath() == expected);
  }

  SECTION("langpack") {
    source.setTypeOverride(Package::LangPackType);
    const Path expected("LangPack/file.name");
    REQUIRE(source.targetPath() == expected);
  }

  SECTION("web interface") {
    source.setTypeOverride(Package::WebInterfaceType);
    const Path expected("reaper_www_root/file.name");
    REQUIRE(source.targetPath() == expected);
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
