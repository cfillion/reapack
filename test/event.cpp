#include "helper.hpp"

#include <event.hpp>

#include <reaper_plugin_functions.h>

static const char *M = "[event]";

TEST_CASE("check whether an event has handlers subscribed to it", M) {
  Event<void()> e;
  REQUIRE_FALSE(e);
  e >> []{};
  REQUIRE(e);
}

TEST_CASE("remove all handlers subscribed to an event", M) {
  bool run = false;
  Event<void()> e;
  e >> [&]{ run = true; };

  e.reset();
  CHECK_FALSE(e);

  e();
  REQUIRE_FALSE(run);
}

TEST_CASE("Event<void(...)> handlers are run in order", M) {
  std::vector<int> bucket;

  Event<void()> e;
  e >> [&] { bucket.push_back(1); }
    >> [&] { bucket.push_back(2); };

  e();

  REQUIRE(bucket == decltype(bucket){1, 2});
}

TEST_CASE("Event<R(...)> handlers are run in order", M) {
  std::vector<int> bucket;

  Event<std::nullptr_t()> e;
  e >> [&] { bucket.push_back(1); return nullptr; }
    >> [&] { bucket.push_back(2); return nullptr; };

  e();

  REQUIRE(bucket == decltype(bucket){1, 2});
}

TEST_CASE("Event<R(...)> returns the last value") {
  Event<int()> e;
  REQUIRE_FALSE(e().has_value());

  e >> [] { return 1; }
    >> [] { return 2; }
    >> [] { return 3; };

  REQUIRE(e().has_value());
  REQUIRE(*e() == 3);
}

TEST_CASE("Event<void(...)> arguments are not copied more than necessary", M) {
  auto obj = std::make_shared<std::nullptr_t>();
  Event<void(decltype(obj), decltype(obj))> e;

  e >> [&obj](decltype(obj) a, decltype(obj) b) {
    // original copy + by-value parameter copies
    REQUIRE(obj.use_count() == 1 + 2);
  };

  e(obj, obj);
};

TEST_CASE("Event<R(...)> arguments are not copied more than necessary", M) {
  auto obj = std::make_shared<std::nullptr_t>();
  Event<std::nullptr_t(decltype(obj), decltype(obj))> e;

  e >> [&obj](decltype(obj) a, decltype(obj) b) {
    REQUIRE(obj.use_count() == 1 + 2);
    return nullptr;
  };

  e(obj, obj);
};

TEST_CASE("multiple AsyncEvent registers a single timer", M) {
  static std::vector<std::string> registered;

  plugin_register = [](const char *n, void *) {
    registered.push_back(n);
    return 0;
  };

  {
    AsyncEvent<void()> e1, e2;
    REQUIRE(registered == decltype(registered){"timer"});
  }

  REQUIRE(registered == decltype(registered){"timer", "-timer"});
}

TEST_CASE("AsyncEvent handlers are run asynchronously in order", M) {
  static void (*tick)() = nullptr;
  plugin_register = [](const char *, void *c) { tick = (void(*)())c; return 0; };

  std::vector<size_t> bucket;

  AsyncEvent<void(size_t)> e;
  e >> [&](size_t i) { bucket.push_back(i); }
    >> [&](size_t i) { bucket.push_back(i * 10); };

  e(1);
  e(2);
  e(3);

  CHECK(bucket.empty());
  tick();
  REQUIRE(bucket == decltype(bucket){1, 10, 2, 20, 3, 30});
}

TEST_CASE("AsyncEvent<void(...)> sets the future's value", M) {
  static void (*tick)() = nullptr;
  plugin_register = [](const char *, void *c) { tick = (void(*)())c; return 0; };

  AsyncEvent<void()> e;
  e >> []{};

  std::future<void> ret = e();

  REQUIRE(ret.wait_for(std::chrono::seconds(0)) == std::future_status::timeout);
  tick();
  REQUIRE(ret.wait_for(std::chrono::seconds(0)) == std::future_status::ready);
}

TEST_CASE("AsyncEvent<R(...)> sets the future's value", M) {
  static void (*tick)() = nullptr;
  plugin_register = [](const char *, void *c) { tick = (void(*)())c; return 0; };

  AsyncEvent<std::string()> e;
  e >> []{ return "hello world"; }
    >> []{ return "foo bar"; };

  std::future<std::optional<std::string>> ret = e();

  REQUIRE(ret.wait_for(std::chrono::seconds(0)) == std::future_status::timeout);
  tick();
  REQUIRE(ret.wait_for(std::chrono::seconds(0)) == std::future_status::ready);

  std::optional<std::string> val = ret.get();
  REQUIRE(val.has_value());
  REQUIRE(*val == "foo bar");
}

TEST_CASE("running an AsyncEvent without handlers returns synchronously", M) {
  plugin_register = [](const char *, void *c) { return 0; };

  AsyncEvent<void()> e1;
  AsyncEvent<int()> e2;

  auto r1 = e1();
  auto r2 = e2();

  REQUIRE(r1.wait_for(std::chrono::seconds(0)) == std::future_status::ready);
  REQUIRE_FALSE(r2.get().has_value());
}

TEST_CASE("AsyncEvent<void(...)> arguments are not copied more than necessary", M) {
  static void (*tick)() = nullptr;
  plugin_register = [](const char *, void *c) { tick = (void(*)())c; return 0; };

  auto obj = std::make_shared<std::nullptr_t>();
  AsyncEvent<void(decltype(obj), decltype(obj))> e;

  e >> [&obj](decltype(obj) a, decltype(obj) b) {
    REQUIRE(obj.use_count() == 1 + 2 + 2);
  };

  e(obj, obj);
  tick();
};

TEST_CASE("AsyncEvent<R(...)> arguments are not copied more than necessary", M) {
  static void (*tick)() = nullptr;
  plugin_register = [](const char *, void *c) { tick = (void(*)())c; return 0; };

  auto obj = std::make_shared<std::nullptr_t>();
  AsyncEvent<std::nullptr_t(decltype(obj), decltype(obj))> e;

  e >> [&obj](decltype(obj) a, decltype(obj) b) {
    // original copy + async copies + by-value parameter copies
    REQUIRE(obj.use_count() == 1 + 2 + 2);
    return nullptr;
  };

  e(obj, obj);
  tick();
};

TEST_CASE("deleted not-yet-posted AsyncEvents are discarded", M) {
  static void (*tick)() = nullptr;
  plugin_register = [](const char *, void *c) { tick = (void(*)())c; return 0; };

  AsyncEvent<void()> keepTimerAlive;

  {
    AsyncEvent<void()> e;
    e();
  }

  tick();
}

TEST_CASE("deleting AsyncEvent from handler is safe", M) {
  static void (*tick)() = nullptr;
  plugin_register = [](const char *, void *c) { tick = (void(*)())c; return 0; };

  auto e = new AsyncEvent<void()>();
  *e >> [e] { delete e; };
  (*e)();

  tick();
}
