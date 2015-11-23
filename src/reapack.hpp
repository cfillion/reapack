#ifndef REAPACK_REAPACK_HPP
#define REAPACK_REAPACK_HPP

#include "reaper_plugin.h"

struct ReaPack {
  ReaPack();

  int actionId() const { return action.accel.cmd; }
  void setActionId(const int id) { action.accel.cmd = id; }

  REAPER_PLUGIN_HINSTANCE instance = 0;
  gaccel_register_t action;
};

#endif
