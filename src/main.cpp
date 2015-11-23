#include "reapack.hpp"

#define REAPERAPI_IMPLEMENT
#include "reaper_plugin_functions.h"

static ReaPack reapack;

bool commandHook(const int command, const int flag)
{
  if(command != reapack.actionId())
    return false;

  ShowMessageBox("Hello World!", "Test", 0);
  return true;
}

extern "C" REAPER_PLUGIN_DLL_EXPORT int REAPER_PLUGIN_ENTRYPOINT(
  REAPER_PLUGIN_HINSTANCE instance, reaper_plugin_info_t *rec)
{
  reapack.instance = instance;

  if(!rec || rec->caller_version != REAPER_PLUGIN_VERSION || !rec->GetFunc)
    return 0;

  if(REAPERAPI_LoadAPI(rec->GetFunc) > 0)
    return 0;

  rec->Register("hookcommand", (void *)commandHook);

  reapack.setActionId(rec->Register("command_id", (void *)"REAPACKMGR"));
  rec->Register("gaccel", &reapack.action);

  return 1;
}
