#include "helper.hpp"

#include <receipt.hpp>

#include <index.hpp>

using Catch::Matchers::Contains;

using namespace std;

static constexpr const char *M = "[receipt]";

TEST_CASE("non-empty receipt", M) {
  Receipt r;
  REQUIRE(r.empty());

  SECTION("install") {
    IndexPtr ri = make_shared<Index>("Index Name");
    Category cat("Category Name", ri.get());
    Package pkg(Package::ScriptType, "Package Name", &cat);
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

TEST_CASE("set RestartNeeded flag", M) {
  IndexPtr ri = make_shared<Index>("Index Name");
  Category cat("Category Name", ri.get());
  Package script(Package::ScriptType, "Package Name", &cat);
  Package ext(Package::ExtensionType, "Package Name", &cat);
  Version scriptVer("1.0", &script);
  Version extVer("1.0", &ext);

  Receipt r;
  REQUIRE_FALSE(r.test(Receipt::RestartNeeded));

  r.addInstall(&scriptVer, {});
  REQUIRE_FALSE(r.test(Receipt::RestartNeeded));

  r.addInstall(&extVer, {});
  REQUIRE(r.test(Receipt::RestartNeeded));
}

TEST_CASE("format receipt page title", M) {
  SECTION("singular") {
    ReceiptPage page{vector<int>{1}, "Singular", "Plural"};
    REQUIRE(page.title() == "Singular (1)");
  }

  SECTION("plural") {
    ReceiptPage page{vector<int>{1, 2, 3}, "Singular", "Plural"};
    REQUIRE(page.title() == "Plural (3)");
  }

  SECTION("zero is plural") {
    ReceiptPage page{vector<int>{}, "Singular", "Plural"};
    REQUIRE(page.title() == "Plural (0)");
  }

  SECTION("no plural") {
    ReceiptPage page{vector<int>{1, 2, 3}, "Fallback"};
    REQUIRE(page.title() == "Fallback (3)");
  }

  SECTION("thousand divider") {
    ReceiptPage page{vector<int>(42'000, 42), "Singular", "Plural"};
    REQUIRE(page.title() == "Plural (42,000)");
  }
}

TEST_CASE("format install ticket") {
  IndexPtr ri = make_shared<Index>("Index Name");
  Category cat("Category Name", ri.get());
  Package pkg(Package::ScriptType, "Package Name", &cat);

  Version *v1 = new Version("1.0", &pkg);
  v1->addSource(new Source({}, "https://google.com", v1));
  pkg.addVersion(v1);

  Version *v2 = new Version("2.0", &pkg);
  v2->addSource(new Source({}, "https://google.com", v2));
  pkg.addVersion(v2);

  Version *v3 = new Version("3.0", &pkg);
  v3->addSource(new Source({}, "https://google.com", v3));
  pkg.addVersion(v3);

  ostringstream stream;
  Registry::Entry entry{1};

  SECTION("contains fullname") {
    stream << InstallTicket{v3, {}};
    REQUIRE_THAT(stream.str(), Contains(pkg.fullName()));
  }

  SECTION("prepend newline if stream nonempty") {
    stream << "something";
    stream << InstallTicket{v3, {}};
    REQUIRE_THAT(stream.str(), Contains("something\r\n"));
  }

  SECTION("installed from scratch") {
    stream << InstallTicket{v2, {}};
    REQUIRE_THAT(stream.str(),
      !Contains("v1.0") && Contains("v2.0") && !Contains("v3.0"));
  }

  SECTION("update") {
    entry.version = VersionName("1.0");
    stream << InstallTicket{v3, entry};
    REQUIRE_THAT(stream.str(),
      !Contains("v1.0") && Contains("v2.0") && Contains("v3.0"));
  }

  SECTION("downgrade") {
    entry.version = VersionName("3.0");
    stream << InstallTicket{v1, entry};
    REQUIRE_THAT(stream.str(),
      Contains("v1.0") && !Contains("v2.0") && !Contains("v3.0"));
  }
}
