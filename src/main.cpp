#include "reapack.hpp"
#include "download.hpp"

#include <cstdlib>

#define REAPERAPI_IMPLEMENT
#include <reaper_plugin_functions.h>

static ReaPack reapack;

bool commandHook(const int id, const int flag)
{
  return reapack.execActions(id, flag);
}

extern "C" REAPER_PLUGIN_DLL_EXPORT int REAPER_PLUGIN_ENTRYPOINT(
  REAPER_PLUGIN_HINSTANCE instance, reaper_plugin_info_t *rec)
{
  if(!rec || rec->caller_version != REAPER_PLUGIN_VERSION || !rec->GetFunc)
    return 0;

  if(REAPERAPI_LoadAPI(rec->GetFunc) > 0)
    return 0;

  printf("%s\n", GetResourcePath());

  Download *down = new Download("http://cfillion.tk/");
  down->addCallback([=](const int status, const char *contents) {
    char strStatus[5];
    sprintf(strStatus, "%zu", strlen(contents));

    ShowMessageBox(contents, strStatus, 0);
    down->start();
  });
  down->start();

  reapack.init(instance, rec);

  reapack.setupAction("REAPACKMGR", "ReaPack: Package Manager",
    &reapack.action, std::bind(&ReaPack::toggleWindow, reapack));

  rec->Register("hookcommand", (void *)commandHook);

  return 1;
}
