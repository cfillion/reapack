#include "helper.hpp"

#include <event.hpp>

#include <reaper_plugin_functions.h>

static constexpr const char *M = "[event]";

TEST_CASE("multiple EventEmitter registers a single timer", M) {
  static std::vector<std::string> registered;

  plugin_register = [](const char *n, void *) {
    registered.push_back(n);
    return 0;
  };

  {
    struct : EventEmitter { void eventHandler(Event) {} } e1, e2;
    REQUIRE(registered == std::vector<std::string>{"timer"});
  }

  REQUIRE(registered == std::vector<std::string>{"timer", "-timer"});
}

TEST_CASE("EventEmitter calls eventHandler from timer and in order", M) {
  static void (*tick)() = nullptr;
  plugin_register = [](const char *, void *c) { tick = (void(*)())c; return 0; };

  struct : EventEmitter {
    void eventHandler(Event e) { bucket.push_back(e); }
    std::vector<Event> bucket;
  } e;

  e.emit(1);
  e.emit(2);
  e.emit(3);
  CHECK(e.bucket.empty());
  tick();
  REQUIRE(e.bucket == std::vector<Event>{1, 2, 3});
}

TEST_CASE("events from deleted EventEmmiter are discarded", M) {
  static void (*tick)() = nullptr;
  plugin_register = [](const char *, void *c) { tick = (void(*)())c; return 0; };

  struct Mock : EventEmitter { void eventHandler(Event) {} };

  Mock a; // keep the timer alive

  Mock *b = new Mock();
  b->emit(0);
  delete b;
  memset((void *)b, 0, sizeof(Mock));

  tick();
}

TEST_CASE("deleting EventEmmiter from eventHandler is safe", M) {
  static void (*tick)() = nullptr;
  plugin_register = [](const char *, void *c) { tick = (void(*)())c; return 0; };

  struct Mock : EventEmitter { void eventHandler(Event) { delete this; } };

  (new Mock())->emit(0); // only one event!
  tick();
}
