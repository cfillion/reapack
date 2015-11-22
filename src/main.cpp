#include "global.hpp"

#define REAPERAPI_IMPLEMENT
#include "reaper_plugin_functions.h"

static Global global;

bool commandHook(const int command, const int flag)
{
  if(command != global.action.accel.cmd)
    return false;

  ShowMessageBox("Hello World!", "Test", 0);
  return true;
}

extern "C" REAPER_PLUGIN_DLL_EXPORT int REAPER_PLUGIN_ENTRYPOINT(
  REAPER_PLUGIN_HINSTANCE instance, reaper_plugin_info_t *rec)
{
  global.instance = instance;
  global.action.desc = "ReaPack: Package Manager";

  if(!rec || rec->caller_version != REAPER_PLUGIN_VERSION || !rec->GetFunc)
    return 0;

  if(REAPERAPI_LoadAPI(rec->GetFunc) > 0)
    return 0;

  rec->Register("hookcommand", (void *)commandHook);

  global.action.accel.cmd = rec->Register("command_id", (void *)"REAPACKMGR");
  rec->Register("gaccel", &global.action);

  return 1;
}
