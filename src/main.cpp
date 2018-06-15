/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2018  Christian Fillion
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "api.hpp"
#include "errors.hpp"
#include "menu.hpp"
#include "reapack.hpp"
#include "win32.hpp"

#define REAPERAPI_IMPLEMENT
#include <reaper_plugin_functions.h>

using namespace std;

#define REQUIRED_API(name) {(void **)&name, #name, true}
#define OPTIONAL_API(name) {(void **)&name, #name, false}

static bool loadAPI(void *(*getFunc)(const char *))
{
  struct ApiFunc { void **ptr; const char *name; bool required; };

  const ApiFunc funcs[] = {
    REQUIRED_API(Splash_GetWnd),             // v4.7

    REQUIRED_API(AddExtensionsMainMenu),
    REQUIRED_API(GetAppVersion),
    REQUIRED_API(GetMainHwnd),
    REQUIRED_API(GetResourcePath),
    REQUIRED_API(NamedCommandLookup),        // v3.1415
    REQUIRED_API(plugin_register),
    REQUIRED_API(ShowMessageBox),

    OPTIONAL_API(AddRemoveReaScript),        // v5.12
  };

  for(const ApiFunc &func : funcs) {
    *func.ptr = getFunc(func.name);

    if(func.required && *func.ptr == nullptr) {
      char msg[1024];
      snprintf(msg, sizeof(msg),
        "ReaPack v%s is incompatible with this version of REAPER.\n\n"
        "(Unable to import the following API function: %s)",
        ReaPack::VERSION, func.name);

      Win32::messageBox(Splash_GetWnd ? Splash_GetWnd() : nullptr,
        msg, "ReaPack: Fatal Error", MB_OK);

      return false;
    }
  }

  return true;
}

static bool commandHook(const int id, const int flag)
{
  return g_reapack->execActions(id, flag);
}

static void menuHook(const char *name, HMENU handle, int f)
{
  if(strcmp(name, "Main extensions") || f != 0)
    return;

  Menu menu = Menu(handle).addMenu("ReaPack");

  menu.addAction("&Synchronize packages",
    NamedCommandLookup("_REAPACK_SYNC"));

  menu.addAction("&Browse packages...",
    NamedCommandLookup("_REAPACK_BROWSE"));

  menu.addAction("&Import repositories...",
    NamedCommandLookup("_REAPACK_IMPORT"));

  menu.addAction("&Manage repositories...",
    NamedCommandLookup("_REAPACK_MANAGE"));

  menu.addSeparator();

  menu.addAction(String::format("&About ReaPack v%s", ReaPack::VERSION),
    NamedCommandLookup("_REAPACK_ABOUT"));
}

static bool checkLocation(REAPER_PLUGIN_HINSTANCE module)
{
  Path expected;
  expected.append(ReaPack::resourcePath());
  expected.append("UserPlugins");
  expected.append(REAPACK_FILE);

#ifdef _WIN32
  Win32::char_type self[MAX_PATH] = {};
  GetModuleFileName(module, self, lengthof(self));
  Path current(Win32::narrow(self));
#else
  Dl_info info{};
  dladdr((const void *)checkLocation, &info);
  const char *self = info.dli_fname;
  Path current(self);
#endif

  if(current == expected)
    return true;

  char msg[4096];
  snprintf(msg, sizeof(msg),
    "ReaPack was not loaded from the standard extension path"
    " or its filename was altered.\n"
    "Move or rename it to the expected location and retry.\n\n"
    "Current: %s\n\nExpected: %s",
    current.join().c_str(), expected.join().c_str());

  Win32::messageBox(Splash_GetWnd(), msg,
    "ReaPack: Installation path mismatch", MB_OK);

  return false;
}

static void setupActions()
{
  g_reapack->setupAction("REAPACK_SYNC", "ReaPack: Synchronize packages",
    &g_reapack->syncAction, bind(&ReaPack::synchronizeAll, g_reapack));

  g_reapack->setupAction("REAPACK_BROWSE", "ReaPack: Browse packages...",
    &g_reapack->browseAction, bind(&ReaPack::browsePackages, g_reapack));

  g_reapack->setupAction("REAPACK_IMPORT", "ReaPack: Import repositories...",
    &g_reapack->importAction, bind(&ReaPack::importRemote, g_reapack));

  g_reapack->setupAction("REAPACK_MANAGE", "ReaPack: Manage repositories...",
    &g_reapack->configAction, bind(&ReaPack::manageRemotes, g_reapack));

  g_reapack->setupAction("REAPACK_ABOUT", bind(&ReaPack::aboutSelf, g_reapack));
}

static void setupAPI()
{
  g_reapack->setupAPI(&API::AboutInstalledPackage);
  g_reapack->setupAPI(&API::AboutRepository);
  g_reapack->setupAPI(&API::AddSetRepository);
  g_reapack->setupAPI(&API::BrowsePackages);
  g_reapack->setupAPI(&API::CompareVersions);
  g_reapack->setupAPI(&API::EnumOwnedFiles);
  g_reapack->setupAPI(&API::FreeEntry);
  g_reapack->setupAPI(&API::GetEntryInfo);
  g_reapack->setupAPI(&API::GetOwner);
  g_reapack->setupAPI(&API::GetRepositoryInfo);
  g_reapack->setupAPI(&API::ProcessQueue);
}

extern "C" REAPER_PLUGIN_DLL_EXPORT int REAPER_PLUGIN_ENTRYPOINT(
  REAPER_PLUGIN_HINSTANCE instance, reaper_plugin_info_t *rec)
{
  if(!rec) {
    plugin_register("-hookcommand", (void *)commandHook);
    plugin_register("-hookcustommenu", (void *)menuHook);

    delete g_reapack;

    return 0;
  }

  if(rec->caller_version != REAPER_PLUGIN_VERSION || !rec->GetFunc)
    return 0;

  if(!loadAPI(rec->GetFunc))
    return 0;

  if(!checkLocation(instance))
    return 0;

  new ReaPack(instance);

  setupActions();
  setupAPI();

  plugin_register("hookcommand", (void *)commandHook);
  plugin_register("hookcustommenu", (void *)menuHook);

  AddExtensionsMainMenu();

  return 1;
}

#ifndef _WIN32
#  include "resource.hpp"
#  include <swell/swell-dlggen.h>
#  include "resource.rc_mac_dlg"
#  include <swell/swell-menugen.h>
#  include "resource.rc_mac_menu"
#endif
