#include "helper.hpp"

#include <receipt.hpp>

#include <index.hpp>

using Catch::Matchers::ContainsSubstring;
using Catch::Matchers::EndsWith;

static const char *M = "[receipt]";

TEST_CASE("non-empty receipt", M) {
  Receipt r;
  REQUIRE(r.empty());

  SECTION("install") {
    IndexPtr ri = std::make_shared<Index>("Index Name");
    Category cat("Category Name", ri.get());
    Package pkg(Package::ScriptType, "Package Name", &cat, "remote");
    Version ver("1.0", &pkg);
    r.addInstall(&ver, {});
  }

  SECTION("removal")
    r.addRemoval(Path("hello/world"));

  SECTION("export")
    r.addExport(Path("hello/world"));

  SECTION("error")
    r.addError({"message", "context"});

  REQUIRE_FALSE(r.empty());
}

TEST_CASE("set receipt flags", M) {
  Receipt r;
  REQUIRE(r.flags() == Receipt::NoFlag);
  Receipt::Flag expected;

  SECTION("install") {
    IndexPtr ri = std::make_shared<Index>("Index Name");
    Category cat("Category Name", ri.get());
    Package pkg(Package::ScriptType, "Package Name", &cat, "remote");
    Version ver("1.0", &pkg);
    r.addInstall(&ver, {});

    CHECK(r.test(Receipt::InstalledOrRemoved));
    CHECK_FALSE(r.test(Receipt::PackageChangedFlag));
    expected = Receipt::InstalledFlag;
  }

  SECTION("removal") {
    r.addRemoval(Path("hello/world"));
    CHECK(r.test(Receipt::InstalledOrRemoved));
    CHECK_FALSE(r.test(Receipt::PackageChangedFlag));
    expected = Receipt::RemovedFlag;
  }

  SECTION("export") {
    r.addExport(Path("hello/world"));
    CHECK_FALSE(r.test(Receipt::InstalledOrRemoved));
    expected = Receipt::ExportedFlag;
  }

  SECTION("error") {
    r.addError({"message", "context"});
    CHECK_FALSE(r.test(Receipt::PackageChangedFlag));
    expected = Receipt::ErrorFlag;
  }

  REQUIRE(r.test(expected));
  REQUIRE_FALSE(r.test(Receipt::NoFlag));
  REQUIRE(r.flags() == expected);
}

TEST_CASE("set restart needed flag", M) {
  IndexPtr ri = std::make_shared<Index>("Index Name");
  Category cat("Category Name", ri.get());
  Package script(Package::ScriptType, "Package Name", &cat, "remote");
  Package ext(Package::ExtensionType, "Package Name", &cat, "remote");
  Version scriptVer("1.0", &script);
  Version extVer("1.0", &ext);

  Receipt r;
  REQUIRE_FALSE(r.test(Receipt::RestartNeededFlag));

  r.addInstall(&scriptVer, {});
  REQUIRE_FALSE(r.test(Receipt::RestartNeededFlag));

  r.addInstall(&extVer, {});
  REQUIRE(r.test(Receipt::RestartNeededFlag));
}

TEST_CASE("set index changed flag", M) {
  Receipt r;
  REQUIRE_FALSE(r.test(Receipt::IndexChangedFlag));
  REQUIRE_FALSE(r.test(Receipt::RefreshBrowser));

  r.setIndexChanged();
  REQUIRE(r.test(Receipt::IndexChangedFlag));
  REQUIRE(r.test(Receipt::RefreshBrowser));
}

TEST_CASE("set package changed flag", M) {
  Receipt r;
  REQUIRE_FALSE(r.test(Receipt::PackageChangedFlag));

  r.setPackageChanged();
  REQUIRE(r.test(Receipt::PackageChangedFlag));
  REQUIRE(r.test(Receipt::RefreshBrowser));
  REQUIRE_FALSE(r.test(Receipt::InstalledOrRemoved));
}

TEST_CASE("format receipt page title", M) {
  SECTION("singular") {
    ReceiptPage page{std::vector<int>{1}, "Singular", "Plural"};
    REQUIRE(page.title() == "Singular (1)");
  }

  SECTION("plural") {
    ReceiptPage page{std::vector<int>{1, 2, 3}, "Singular", "Plural"};
    REQUIRE(page.title() == "Plural (3)");
  }

  SECTION("zero is plural") {
    ReceiptPage page{std::vector<int>{}, "Singular", "Plural"};
    REQUIRE(page.title() == "Plural (0)");
  }

  SECTION("no plural") {
    ReceiptPage page{std::vector<int>{1, 2, 3}, "Fallback"};
    REQUIRE(page.title() == "Fallback (3)");
  }

  SECTION("thousand divider") {
    ReceiptPage page{std::vector<int>(42'000, 42), "Singular", "Plural"};
    REQUIRE(page.title() == "Plural (42,000)");
  }
}

TEST_CASE("format receipt page contents", M) {
  ReceiptPage page{std::vector<int>{1, 2, 3}, "", ""};
  REQUIRE(page.contents() == "1\r\n2\r\n3");
}

TEST_CASE("format install ticket", M) {
  IndexPtr ri = std::make_shared<Index>("Index Name");
  Category cat("Category Name", ri.get());
  Package pkg(Package::ScriptType, "Package Name", &cat, "remote");

  Version *v1 = new Version("1.0", &pkg);
  v1->addSource(new Source({}, "https://google.com", v1));
  pkg.addVersion(v1);

  Version *v2 = new Version("2.0", &pkg);
  v2->addSource(new Source({}, "https://google.com", v2));
  pkg.addVersion(v2);

  Version *v3 = new Version("3.0", &pkg);
  v3->addSource(new Source({}, "https://google.com", v3));
  pkg.addVersion(v3);

  std::ostringstream stream;
  Registry::Entry entry{1};

  SECTION("contains fullname") {
    stream << InstallTicket{v3, {}};
    REQUIRE_THAT(stream.str(), ContainsSubstring(pkg.fullName()));
  }

  SECTION("prepend newline if stream nonempty") {
    stream << "something";
    stream << InstallTicket{v3, {}};
    REQUIRE_THAT(stream.str(), ContainsSubstring("something\r\n"));
  }

  SECTION("installed from scratch") {
    stream << InstallTicket{v2, {}};
    REQUIRE_THAT(stream.str(), ContainsSubstring("[new]") &&
      !ContainsSubstring("v1.0\r\n") && ContainsSubstring("v2.0\r\n") && !ContainsSubstring("v3.0\r\n"));
  }

  SECTION("reinstalled") {
    entry.version = VersionName("2.0");
    stream << InstallTicket{v2, entry};
    REQUIRE_THAT(stream.str(), ContainsSubstring("[reinstalled]") &&
      !ContainsSubstring("v1.0\r\n") && ContainsSubstring("v2.0\r\n") && !ContainsSubstring("v3.0\r\n"));
  }

  SECTION("update") {
    entry.version = VersionName("1.0");
    stream << InstallTicket{v3, entry};
    REQUIRE_THAT(stream.str(), ContainsSubstring("[v1.0 -> v3.0]") &&
      !ContainsSubstring("v1.0\r\n") && ContainsSubstring("\r\nv3.0\r\n  No changelog\r\nv2.0"));
    REQUIRE_THAT(stream.str(), !EndsWith("\r\n"));
  }

  SECTION("downgrade") {
    entry.version = VersionName("3.0");
    stream << InstallTicket{v1, entry};
    REQUIRE_THAT(stream.str(), ContainsSubstring("[v3.0 -> v1.0]") &&
      ContainsSubstring("v1.0\r\n") && !ContainsSubstring("v2.0\r\n") && !ContainsSubstring("v3.0\r\n"));
  }
}

TEST_CASE("sort InstallTickets (case insensitive)", M) {
  IndexPtr ri = std::make_shared<Index>("Index Name");
  Category cat("Category Name", ri.get());
  Package pkg1(Package::ScriptType, "a test", &cat, "remote");
  Version ver1("1.0", &pkg1);

  Package pkg2(Package::ScriptType, "Uppercase Name", &cat, "remote");
  Version ver2("1.0", &pkg2);

  Package pkg3(Package::ScriptType, "unused name", &cat, "remote");
  pkg3.setDescription("z is the last letter");
  Version ver3("1.0", &pkg3);

  REQUIRE(InstallTicket(&ver1, {}) < InstallTicket(&ver2, {}));
  REQUIRE(InstallTicket(&ver2, {}) < InstallTicket(&ver3, {}));
  REQUIRE_FALSE(InstallTicket(&ver1, {}) < InstallTicket(&ver1, {}));
  REQUIRE_FALSE(InstallTicket(&ver2, {}) < InstallTicket(&ver1, {}));

  Receipt r;
  r.addInstall(&ver1, {}); // a test
  r.addInstall(&ver3, {}); // z is the last letter
  r.addInstall(&ver1, {}); // a test (duplicate)
  r.addInstall(&ver2, {}); // Uppercase Name
  const std::string page = r.installedPage().contents();
  REQUIRE(page.find(pkg1.name()) < page.find(pkg2.name()));
  REQUIRE(page.find(pkg2.name()) < page.find(pkg3.name()));

  // duplicates should still be preserved
  REQUIRE(page.find(pkg1.name()) < page.rfind(pkg1.name()));
}
