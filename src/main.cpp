#include "menu.hpp"
#include "reapack.hpp"

#define REAPERAPI_IMPLEMENT
#include <reaper_plugin_functions.h>

using namespace std;

static ReaPack reapack;

static bool commandHook(const int id, const int flag)
{
  return reapack.execActions(id, flag);
}

static void menuHook(const char *name, HMENU handle, int f)
{
  if(strcmp(name, "Main extensions") || f != 0)
    return;

  Menu menu = Menu(handle).addMenu("ReaPack");

  menu.addAction("Synchronize packages",
    NamedCommandLookup("_REAPACK_SYNC"));

  menu.addAction("Import remote repository...",
    NamedCommandLookup("_REAPACK_IMPORT"));
}

extern "C" REAPER_PLUGIN_DLL_EXPORT int REAPER_PLUGIN_ENTRYPOINT(
  REAPER_PLUGIN_HINSTANCE instance, reaper_plugin_info_t *rec)
{
  if(!rec) {
    reapack.cleanup();
    return 0;
  }

  if(rec->caller_version != REAPER_PLUGIN_VERSION || !rec->GetFunc)
    return 0;

  if(REAPERAPI_LoadAPI(rec->GetFunc) > 0)
    return 0;

  reapack.init(instance, rec);

  reapack.setupAction("REAPACK_SYNC", "ReaPack: Synchronize Packages",
    &reapack.syncAction, bind(&ReaPack::synchronize, reapack));

  reapack.setupAction("REAPACK_IMPORT",
    bind(&ReaPack::importRemote, reapack));

  rec->Register("hookcommand", (void *)commandHook);
  rec->Register("hookcustommenu", (void *)menuHook);

  AddExtensionsMainMenu();

  return 1;
}

#ifdef __APPLE__
#include "resource.hpp"

#include <swell/swell-dlggen.h>
#include "resource.rc_mac_dlg"

#include <swell/swell-menugen.h>
#include "resource.rc_mac_menu"
#endif
