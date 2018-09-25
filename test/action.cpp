#include "helper.hpp"

#include <action.hpp>
#include <reaper_plugin_functions.h>

static const char *M = "[action]";

using namespace std::string_literals;

TEST_CASE("registering command ID", M) {
  static const char *commandName = nullptr;

  plugin_register = [](const char *what, void *data) {
    if(!strcmp(what, "command_id")) {
      commandName = static_cast<const char *>(data);
      return 1234;
    }

    return 0;
  };

  const Action action("HELLO", "Hello World", {});
  REQUIRE(action.id() == 1234);
  REQUIRE(commandName == "HELLO"s);
}

TEST_CASE("registering action in REAPER's Action List", M) {
  static gaccel_register_t *reg = nullptr;

  plugin_register = [](const char *what, void *data) {
    if(!strcmp(what, "command_id"))
      return 4321;
    else if(!strcmp(what, "gaccel"))
      reg = static_cast<gaccel_register_t *>(data);

    return 0;
  };

  const Action action("HELLO", "Hello World", {});
  CHECK(reg != nullptr);
  REQUIRE(reg->accel.cmd == 4321);
  REQUIRE(reg->desc == "Hello World"s);
}

TEST_CASE("commands & actions are unregistered on destruction", M) {
  static std::vector<std::string> reglog;
  static std::vector<void *> datalog;

  plugin_register = [](const char *what, void *data) {
    reglog.push_back(what);
    datalog.push_back(data);
    return 0;
  };

  {
    const Action action("HELLO", "Hello World", {});
    REQUIRE(reglog == std::vector<std::string>{"command_id", "gaccel"});
  }

  REQUIRE(reglog == std::vector<std::string>{
    "command_id", "gaccel", "-gaccel", "-command_id"});
  REQUIRE(datalog[0] == datalog[3]);
  REQUIRE(datalog[1] == datalog[2]);
}

TEST_CASE("run action", M) {
  static int commandId = 0;

  plugin_register = [](const char *what, void *data) {
    if(!strcmp(what, "command_id"))
      return ++commandId;
    return 0;
  };

  int a = 0, b = 0;
  ActionList list;
  list.add("Action1", "First action", [&a] { ++a; });
  list.add("Action2", "Second action", [&b] { ++b; });

  REQUIRE(!list.run(0));

  CHECK(a == 0);
  REQUIRE(list.run(1));
  REQUIRE(a == 1);

  CHECK(b == 0);
  REQUIRE(list.run(2));
  REQUIRE(b == 1);

  REQUIRE(!list.run(3));
}
