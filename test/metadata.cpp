#include <catch.hpp>

#include <metadata.hpp>

using namespace std;

static const char *M = "[metadata]";

TEST_CASE("repository links", M) {
  Metadata md;
  CHECK(md.links(Metadata::WebsiteLink).empty());
  CHECK(md.links(Metadata::DonationLink).empty());

  SECTION("website links") {
    md.addLink(Metadata::WebsiteLink, {"First", "http://example.com"});
    REQUIRE(md.links(Metadata::WebsiteLink).size() == 1);
    md.addLink(Metadata::WebsiteLink, {"Second", "http://example.com"});

    const auto &links = md.links(Metadata::WebsiteLink);
    REQUIRE(links.size() == 2);
    REQUIRE(links[0]->name == "First");
    REQUIRE(links[1]->name == "Second");

    REQUIRE(md.links(Metadata::DonationLink).empty());
  }

  SECTION("donation links") {
    md.addLink(Metadata::DonationLink, {"First", "http://example.com"});
    REQUIRE(md.links(Metadata::DonationLink).size() == 1);
  }

  SECTION("drop invalid links") {
    md.addLink(Metadata::WebsiteLink, {"name", "not http(s)"});
    REQUIRE(md.links(Metadata::WebsiteLink).empty());
  }
}

TEST_CASE("link type from stmdng", M) {
  REQUIRE(Metadata::getLinkType("website") == Metadata::WebsiteLink);
  REQUIRE(Metadata::getLinkType("donation") == Metadata::DonationLink);
  REQUIRE(Metadata::getLinkType("bacon") == Metadata::WebsiteLink);
}

TEST_CASE("about text", M) {
  Metadata md;
  CHECK(md.about().empty());

  md.setAbout("Hello World");
  REQUIRE(md.about() == "Hello World");
}
