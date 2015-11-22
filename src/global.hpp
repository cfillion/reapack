#ifndef REAPACK_GLOBAL_HPP
#define REAPACK_GLOBAL_HPP

#include "reaper_plugin.h"

struct Global {
  REAPER_PLUGIN_HINSTANCE instance = 0;
  gaccel_register_t action;
};

#endif
