#include "helper.hpp"

#include <time.hpp>

static const char *M = "[time]";

TEST_CASE("valid time", M) {
  const Time time("2016-02-12T01:16:40Z");
  REQUIRE(time.year() == 2016);
  REQUIRE(time.month() == 2);
  REQUIRE(time.day() == 12);
  REQUIRE(time.hour() == 1);
  REQUIRE(time.minute() == 16);
  REQUIRE(time.second() == 40);
  REQUIRE(time == Time(2016, 2, 12, 1, 16, 40));
  REQUIRE(time.isValid());
  REQUIRE(time);
}

TEST_CASE("garbage time string", M) {
  const Time time("hello world");
  REQUIRE_FALSE(time.isValid());
  REQUIRE_FALSE(time);
  REQUIRE(time == Time());
  REQUIRE(time.toString() == "");
}

TEST_CASE("out of range time string", M) {
  const Time time("2016-99-99T99:99:99Z");
  REQUIRE_FALSE(time.isValid());
  REQUIRE_FALSE(time);
  REQUIRE(time == Time());
  REQUIRE(time.toString() == "");
}

TEST_CASE("compare times", M) {
  SECTION("equality") {
    REQUIRE(Time(2016,1,2,3,4,5).compare(Time(2016,1,2,3,4,5)) == 0);

    REQUIRE(Time(2016,1,2,3,4,5) == Time(2016,1,2,3,4,5));
    REQUIRE_FALSE(Time(2016,1,2,3,4,5) == Time(2016,1,2,3,4));
  }

  SECTION("inequality") {
    REQUIRE_FALSE(Time(2016,1,2,3,4,5) != Time(2016,1,2,3,4,5));
    REQUIRE(Time(2016,1,2,3,4,5) != Time(2017,5,4,3,2,1));
  }

  SECTION("less than") {
    REQUIRE(Time(2015, 2, 3).compare(Time(2016, 2, 3)) == -1);

    REQUIRE(Time(2015, 2, 3) < Time(2016, 2, 3));
    REQUIRE_FALSE(Time(2016, 2, 3) < Time(2016, 2, 3));
    REQUIRE_FALSE(Time(2016, 2, 1) < Time(2015, 2, 2));
  }

  SECTION("less than or equal") {
    REQUIRE(Time(2015, 2, 3) <= Time(2016, 2, 3));
    REQUIRE(Time(2016, 2, 3) <= Time(2016, 2, 3));
    REQUIRE_FALSE(Time(2016, 2, 1) <= Time(2015, 2, 2));
  }

  SECTION("greater than") {
    REQUIRE(Time(2016, 2, 3).compare(Time(2015, 2, 3)) == 1);

    REQUIRE_FALSE(Time(2015, 2, 30) > Time(2016, 2, 3));
    REQUIRE_FALSE(Time(2016, 2, 3) > Time(2016, 2, 3));
    REQUIRE(Time(2016, 2, 3) > Time(2015, 2, 3));
  }

  SECTION("greater than or equal") {
    REQUIRE_FALSE(Time(2015, 2, 30) >= Time(2016, 2, 3));
    REQUIRE(Time(2016, 2, 3) >= Time(2016, 2, 3));
    REQUIRE(Time(2016, 2, 3) >= Time(2015, 2, 3));
  }
}
