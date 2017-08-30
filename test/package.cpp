#include "helper.hpp"

#include <errors.hpp>
#include <index.hpp>
#include <package.hpp>

#include <string>

using namespace std;

static const char *M = "[package]";

TEST_CASE("package type from string", M) {
  const vector<pair<const char *, Package::Type> > tests{
    {"yoyo", Package::UnknownType},
    {"script", Package::ScriptType},
    {"extension", Package::ExtensionType},
    {"effect", Package::EffectType},
    {"data", Package::DataType},
    {"theme", Package::ThemeType},
    {"langpack", Package::LangPackType},
    {"webinterface", Package::WebInterfaceType},
    {"projecttpl", Package::ProjectTemplateType},
    {"tracktpl", Package::TrackTemplateType},
    {"midinotenames", Package::MIDINoteNamesType},
  };

  for(const auto &pair : tests)
    REQUIRE(Package::getType(pair.first) == pair.second);
}

TEST_CASE("package type to string", M) {
  const vector<pair<Package::Type, String> > tests{
    {Package::UnknownType,           L"Unknown"},
    {Package::ScriptType,            L"Script"},
    {Package::ExtensionType,         L"Extension"},
    {Package::EffectType,            L"Effect"},
    {Package::DataType,              L"Data"},
    {Package::ThemeType,             L"Theme"},
    {Package::LangPackType,          L"Language Pack"},
    {Package::WebInterfaceType,      L"Web Interface"},
    {Package::ProjectTemplateType,   L"Project Template"},
    {Package::TrackTemplateType,     L"Track Template"},
    {Package::MIDINoteNamesType,     L"MIDI Note Names"},
    {static_cast<Package::Type>(-1), L"Unknown"},
  };

  for(const auto &pair : tests)
    REQUIRE(Package::displayType(pair.first) == pair.second);
}

TEST_CASE("invalid package name", M) {
  SECTION("empty") {
    try {
      Package pack(Package::ScriptType, {}, nullptr);
      FAIL();
    }
    catch(const reapack_error &e) {
      REQUIRE(e.what() == L"empty package name");
    }
  }

  SECTION("slash") {
    try {
      Package pack(Package::ScriptType, "hello/world", nullptr);
      FAIL();
    }
    catch(const reapack_error &e) {
      REQUIRE(e.what() == L"invalid package name 'hello/world'");
    }
  }

  SECTION("backslash") {
    try {
      Package pack(Package::ScriptType, "hello\\world", nullptr);
      FAIL();
    }
    catch(const reapack_error &e) {
      REQUIRE(e.what() == L"invalid package name 'hello\\world'");
    }
  }
}

TEST_CASE("package versions are sorted", M) {
  Index ri("Remote Name");
  Category cat("Category Name", &ri);

  Package pack(Package::ScriptType, "a", &cat);
  CHECK(pack.versions().size() == 0);

  Version *final = new Version("1", &pack);
  final->addSource(new Source({}, "google.com", final));

  Version *alpha = new Version("0.1", &pack);
  alpha->addSource(new Source({}, "google.com", alpha));

  REQUIRE(pack.addVersion(final));
  REQUIRE(final->package() == &pack);
  CHECK(pack.versions().size() == 1);

  pack.addVersion(alpha);
  CHECK(pack.versions().size() == 2);

  REQUIRE(pack.version(0) == alpha);
  REQUIRE(pack.version(1) == final);
  REQUIRE(pack.lastVersion() == final);
}

TEST_CASE("get latest stable version", M) {
  Index ri("Remote Name");
  Category cat("Category Name", &ri);
  Package pack(Package::ScriptType, "a", &cat);

  Version *alpha = new Version("2.0-alpha", &pack);
  alpha->addSource(new Source({}, "google.com", alpha));
  REQUIRE(pack.addVersion(alpha));

  SECTION("only prereleases are available")
    REQUIRE(pack.lastVersion(false) == nullptr);

  SECTION("an older stable release is available") {
    Version *final = new Version("1.0", &pack);
    final->addSource(new Source({}, "google.com", final));
    pack.addVersion(final);

    REQUIRE(pack.lastVersion(false) == final);
  }

  REQUIRE(pack.lastVersion() == alpha);
  REQUIRE(pack.lastVersion(true) == alpha);
}

TEST_CASE("pre-release updates", M) {
  Index ri("Remote Name");
  Category cat("Category Name", &ri);
  Package pack(Package::ScriptType, "a", &cat);

  Version *stable1 = new Version("0.9", &pack);
  stable1->addSource(new Source({}, "google.com", stable1));
  pack.addVersion(stable1);

  Version *alpha1 = new Version("1.0-alpha1", &pack);
  alpha1->addSource(new Source({}, "google.com", alpha1));
  pack.addVersion(alpha1);

  Version *alpha2 = new Version("1.0-alpha2", &pack);
  alpha2->addSource(new Source({}, "google.com", alpha2));
  pack.addVersion(alpha2);

  SECTION("pre-release to next pre-release")
    REQUIRE(pack.lastVersion(false, {"1.0-alpha1"}) == alpha2);

  SECTION("pre-release to latest stable") {
    Version *stable2 = new Version("1.0", &pack);
    stable2->addSource(new Source({}, "google.com", stable2));
    pack.addVersion(stable2);

    Version *stable3 = new Version("1.1", &pack);
    stable3->addSource(new Source({}, "google.com", stable3));
    pack.addVersion(stable3);

    Version *beta = new Version("2.0-beta", &pack);
    beta->addSource(new Source({}, "google.com", beta));
    pack.addVersion(beta);

    REQUIRE(pack.lastVersion(false, {"1.0-alpha1"}) == stable3);
  }
}

TEST_CASE("drop empty version", M) {
  Package pack(Package::ScriptType, "a", nullptr);
  const Version ver("1", &pack);
  REQUIRE_FALSE(pack.addVersion(&ver));
  REQUIRE(pack.versions().empty());
  REQUIRE(pack.lastVersion() == nullptr);
}

TEST_CASE("add owned version", M) {
  Package pack1(Package::ScriptType, "a", nullptr);
  Package pack2(Package::ScriptType, "a", nullptr);

  Version *ver = new Version("1", &pack1);

  try {
    pack2.addVersion(ver);
    FAIL();
  }
  catch(const reapack_error &e) {
    delete ver;
    REQUIRE(e.what() == L"version belongs to another package");
  }
}

TEST_CASE("add duplicate version", M) {
  Index ri("r");
  Category cat("c", &ri);
  Package pack(Package::ScriptType, "p", &cat);

  Version *ver = new Version("1", &pack);
  ver->addSource(new Source({}, "google.com", ver));
  pack.addVersion(ver);

  try {
    pack.addVersion(ver); // could also be an equivalent version with same value
    FAIL();
  }
  catch(const reapack_error &e) {
    REQUIRE(e.what() == L"duplicate version 'r/c/p v1'");
  }
}

TEST_CASE("find matching version", M) {
  Index ri("Remote Name");
  Category cat("Category Name", &ri);

  Package pack(Package::ScriptType, "a", &cat);
  CHECK(pack.versions().size() == 0);

  Version *ver = new Version("1", &pack);
  ver->addSource(new Source({}, "google.com", ver));

  REQUIRE(pack.findVersion({"1"}) == nullptr);
  REQUIRE(pack.findVersion({"2"}) == nullptr);

  pack.addVersion(ver);

  REQUIRE(pack.findVersion({"1"}) == ver);
  REQUIRE(pack.findVersion({"2"}) == nullptr);
}

TEST_CASE("package full name", M) {
  const Index ri("Index Name");
  const Category cat("Category Name", &ri);
  Package pack(Package::ScriptType, "file.name", &cat);

  REQUIRE(pack.fullName() == L"Index Name/Category Name/file.name");

  pack.setDescription("Hello World");
  REQUIRE(pack.fullName() == L"Index Name/Category Name/Hello World");
}

TEST_CASE("package description", M) {
  Package pack(Package::ScriptType, "test.lua", nullptr);
  REQUIRE(pack.description().empty());

  pack.setDescription("hello world");
  REQUIRE(pack.description() == L"hello world");
}

TEST_CASE("package display name", M) {
  Package pack(Package::ScriptType, "test.lua", nullptr);
  REQUIRE(pack.displayName() == L"test.lua");

  pack.setDescription("hello world");
  REQUIRE(pack.displayName() == L"hello world");
}
