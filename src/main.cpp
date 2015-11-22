#define REAPERAPI_IMPLEMENT
#include "reaper_plugin_functions.h"

bool commandHook(int command, int flag)
{
  ShowMessageBox("Hello World!", "Test", 0);
  return true;
}

extern "C" REAPER_PLUGIN_DLL_EXPORT int REAPER_PLUGIN_ENTRYPOINT(
  REAPER_PLUGIN_HINSTANCE instance, reaper_plugin_info_t *rec)
{
  if(!rec || rec->caller_version != REAPER_PLUGIN_VERSION || !rec->GetFunc)
    return 0;

  if(REAPERAPI_LoadAPI(rec->GetFunc) > 0)
    return 0;

  rec->Register("hookcommand", (void *)commandHook);

  return 1;
}
