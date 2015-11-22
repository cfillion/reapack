#include "reaper_plugin.h"


extern "C" REAPER_PLUGIN_DLL_EXPORT int REAPER_PLUGIN_ENTRYPOINT(
  REAPER_PLUGIN_HINSTANCE instance, reaper_plugin_info_t *rec)
{
  if(!rec || rec->caller_version != REAPER_PLUGIN_VERSION || !rec->GetFunc)
    return 0;

  return 1;
}
