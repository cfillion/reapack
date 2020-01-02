#include "helper.hpp"

#include <xml.hpp>

#include <cstring>
#include <sstream>

static const char *M = "[xml]";

using namespace std::string_literals;

TEST_CASE("read empty document", M) {
  std::stringstream s;
  XmlDocument doc(s);
  REQUIRE((!doc || !doc.root()));
}

TEST_CASE("read invalid document", M) {
  std::stringstream s("<unclosed_tag>");
  XmlDocument doc(s);
  REQUIRE(!doc);
  REQUIRE(!doc.root());
  REQUIRE(strlen(doc.error()) > 0);
}

TEST_CASE("valid document has no errors", M) {
  std::stringstream s("<root></root>");
  XmlDocument doc(s);
  REQUIRE(doc);
  REQUIRE(doc.root());
  REQUIRE(doc.error() == nullptr);
}

static XmlDocument parse(const char *data)
{
  std::stringstream s(data);

  XmlDocument doc(s);
  CHECK(doc);
  CHECK(doc.root());

  if(doc.error())
    FAIL(doc.error());

  return doc;
}

TEST_CASE("read element name", M) {
  const XmlDocument &doc = parse("<foobar></foobar>");
  REQUIRE(doc.root().name() == "foobar"s);
}

TEST_CASE("read string attribute", M) {
  const XmlDocument &doc = parse(R"(<root foo="bar" bar=""></root>)");

  SECTION("exists") {
    REQUIRE(doc.root().attribute("foo"));
    REQUIRE(*doc.root().attribute("foo") == "bar"s);
  }

  SECTION("no value") {
    REQUIRE(doc.root().attribute("bar"));
    REQUIRE(*doc.root().attribute("bar") == ""s);
  }

  SECTION("missing") {
    REQUIRE(!doc.root().attribute("baz"));
    REQUIRE(*doc.root().attribute("baz") == nullptr);
  }
}

TEST_CASE("read integer attribute", M) {
  const XmlDocument &doc = parse(R"(<root foo="123" bar="baz"></root>)");
  int output = 42'42'42;

  SECTION("numeric value") {
    REQUIRE(doc.root().attribute("foo", &output));
    REQUIRE(output == 123);
  }

  SECTION("failed conversion") {
    REQUIRE(!doc.root().attribute("bar", &output));
    REQUIRE(output == 42'42'42);
  }

  SECTION("missing attribute") {
    REQUIRE(!doc.root().attribute("baz", &output));
    REQUIRE(output == 42'42'42);
  }
}

TEST_CASE("no element contents", M) {
  const XmlDocument &doc = parse("<root></root>");

  REQUIRE(!doc.root().text());
  REQUIRE(*doc.root().text() == nullptr);
}

TEST_CASE("read element contents", M) {
  const XmlDocument &doc = parse("<root>Hello\nWorld!</root>");

  REQUIRE(doc.root().text());
  REQUIRE(*doc.root().text() == "Hello\nWorld!"s);
}

TEST_CASE("read element CDATA", M) {
  const XmlDocument &doc = parse("<root><![CDATA[Hello\nWorld!]]>\n\x20\x20</root>");

  REQUIRE(doc.root().text());
  REQUIRE(*doc.root().text() == "Hello\nWorld!"s);
}

TEST_CASE("get first child element", M) {
  const XmlDocument &doc = parse(R"(
<root>
  <foo>
    <chunky></chunky>
  </foo>
  <bar>
    <bacon></bacon>
  </bar>
</root>)");

  SECTION("first child") {
    REQUIRE(doc.root().firstChild());
    REQUIRE(doc.root().firstChild().name() == "foo"s);
  }

  SECTION("find by name") {
    REQUIRE(doc.root().firstChild("bar"));
    REQUIRE(doc.root().firstChild("bar").name() == "bar"s);
  }
}

TEST_CASE("get next sibling element", M) {
  const XmlDocument &doc = parse(R"(
<root>
  <foo>
    <chunky></chunky>
    <bacon></bacon>
  </foo>
  <bar>
    <hello></hello>
    <world></world>
  </bar>
  <baz>
    <coffee></coffee>
  </baz>
</root>)");

  const XmlNode &foo = doc.root().firstChild();

  SECTION("first sibling") {
    REQUIRE(foo.nextSibling());
    REQUIRE(foo.nextSibling().name() == "bar"s);
  }

  SECTION("find by name") {
    REQUIRE(foo.nextSibling("baz"));
    REQUIRE(foo.nextSibling("baz").name() == "baz"s);
  }
}
