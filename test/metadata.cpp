#include "helper.hpp"

#include <metadata.hpp>

static const char *M = "[metadata]";

TEST_CASE("repository links", M) {
  Metadata md;
  CHECK(md.links().empty());

  SECTION("website links") {
    md.addLink(Metadata::WebsiteLink, {"First", "http://example.com"});
    REQUIRE(md.links().count(Metadata::WebsiteLink) == 1);
    md.addLink(Metadata::WebsiteLink, {"Second", "http://example.com"});

    auto link = md.links().begin();
    REQUIRE(link->first == Metadata::WebsiteLink);
    REQUIRE(link->second.name == "First");
    REQUIRE((++link)->second.name == "Second");

    REQUIRE(md.links().count(Metadata::DonationLink) == 0);
  }

  SECTION("donation links") {
    md.addLink(Metadata::DonationLink, {"First", "http://example.com"});
    REQUIRE(md.links().count(Metadata::DonationLink) == 1);
    REQUIRE(md.links().begin()->first == Metadata::DonationLink);
  }

  SECTION("drop invalid links") {
    md.addLink(Metadata::WebsiteLink, {"name", "not http(s)"});
    REQUIRE(md.links().count(Metadata::WebsiteLink) == 0);
  }
}

TEST_CASE("link type from stmdng", M) {
  REQUIRE(Metadata::getLinkType("website") == Metadata::WebsiteLink);
  REQUIRE(Metadata::getLinkType("donation") == Metadata::DonationLink);
  REQUIRE(Metadata::getLinkType("screenshot") == Metadata::ScreenshotLink);
  REQUIRE(Metadata::getLinkType("bacon") == Metadata::WebsiteLink);
}

TEST_CASE("about text", M) {
  Metadata md;
  CHECK(md.about().empty());

  md.setAbout("Hello World");
  REQUIRE(md.about() == "Hello World");
}
