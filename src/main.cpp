/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2017  Christian Fillion
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

#define REAPERAPI_IMPLEMENT
#include <reaper_plugin_functions.h>

using namespace std;

static ReaPack *reapack = nullptr;

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
      auto_char msg[1024];
      auto_snprintf(msg, auto_size(msg),
        AUTO_STR("ReaPack v%s is incompatible with this version of REAPER.\r\n\r\n")
        AUTO_STR("(Unable to import the following API function: %s)"),
        make_autocstring(ReaPack::VERSION), make_autocstring(func.name));

      MessageBox(Splash_GetWnd ? Splash_GetWnd() : nullptr,
        msg, AUTO_STR("ReaPack: Fatal Error"), MB_OK);

      return false;
    }
  }

  return true;
}

static bool commandHook(const int id, const int flag)
{
  return reapack->execActions(id, flag);
}

static void menuHook(const char *name, HMENU handle, int f)
{
  if(strcmp(name, "Main extensions") || f != 0)
    return;

  Menu menu = Menu(handle).addMenu(AUTO_STR("ReaPack"));

  menu.addAction(AUTO_STR("&Synchronize packages"),
    NamedCommandLookup("_REAPACK_SYNC"));

  menu.addAction(AUTO_STR("&Browse packages..."),
    NamedCommandLookup("_REAPACK_BROWSE"));

  menu.addAction(AUTO_STR("&Import repositories..."),
    NamedCommandLookup("_REAPACK_IMPORT"));

  menu.addAction(AUTO_STR("&Manage repositories..."),
    NamedCommandLookup("_REAPACK_MANAGE"));

  menu.addSeparator();

  auto_char aboutLabel[32];
  auto_snprintf(aboutLabel, auto_size(aboutLabel),
    AUTO_STR("&About ReaPack v%s"), make_autocstring(ReaPack::VERSION));
  menu.addAction(aboutLabel, NamedCommandLookup("_REAPACK_ABOUT"));
}

static bool checkLocation(REAPER_PLUGIN_HINSTANCE module)
{
  Path expected;
  expected.append(ReaPack::resourcePath());
  expected.append("UserPlugins");
  expected.append(REAPACK_FILE);

#ifdef _WIN32
  auto_char self[MAX_PATH] = {};
  GetModuleFileName(module, self, auto_size(self));
  Path current(from_autostring(self).c_str());
#else
  Dl_info info{};
  dladdr((const void *)checkLocation, &info);
  const char *self = info.dli_fname;
  Path current(self);
#endif

  if(current == expected)
    return true;

  auto_char msg[4096];
  auto_snprintf(msg, auto_size(msg),
    AUTO_STR("ReaPack was not loaded from the standard extension path")
    AUTO_STR(" or its filename was altered.\n")
    AUTO_STR("Move or rename it to the expected location and retry.\n\n")
    AUTO_STR("Current:\xa0%s\n\nExpected:\xa0%s"),
    self, make_autostring(expected.join()).c_str());

  MessageBox(Splash_GetWnd(), msg,
    AUTO_STR("ReaPack: Installation path mismatch"), MB_OK);

  return false;
}

static void setupActions()
{
  reapack->setupAction("REAPACK_SYNC", "ReaPack: Synchronize packages",
    &reapack->syncAction, bind(&ReaPack::synchronizeAll, reapack));

  reapack->setupAction("REAPACK_BROWSE", "ReaPack: Browse packages...",
    &reapack->browseAction, bind(&ReaPack::browsePackages, reapack));

  reapack->setupAction("REAPACK_IMPORT", "ReaPack: Import repositories...",
    &reapack->importAction, bind(&ReaPack::importRemote, reapack));

  reapack->setupAction("REAPACK_MANAGE", "ReaPack: Manage repositories...",
    &reapack->configAction, bind(&ReaPack::manageRemotes, reapack));

  reapack->setupAction("REAPACK_ABOUT", bind(&ReaPack::aboutSelf, reapack));
}

static void setupAPI()
{
  reapack->setupAPI(&API::AboutInstalledPackage);
  reapack->setupAPI(&API::AboutRepository);
  reapack->setupAPI(&API::AddSetRepository);
  reapack->setupAPI(&API::BrowsePackages);
  reapack->setupAPI(&API::CompareVersions);
  reapack->setupAPI(&API::EnumOwnedFiles);
  reapack->setupAPI(&API::FreeEntry);
  reapack->setupAPI(&API::GetEntryInfo);
  reapack->setupAPI(&API::GetOwner);
  reapack->setupAPI(&API::GetRepositoryInfo);
}

extern "C" REAPER_PLUGIN_DLL_EXPORT int REAPER_PLUGIN_ENTRYPOINT(
  REAPER_PLUGIN_HINSTANCE instance, reaper_plugin_info_t *rec)
{
  if(!rec) {
    plugin_register("-hookcommand", (void *)commandHook);
    plugin_register("-hookcustommenu", (void *)menuHook);

    delete reapack;

    return 0;
  }

  if(rec->caller_version != REAPER_PLUGIN_VERSION || !rec->GetFunc)
    return 0;

  if(!loadAPI(rec->GetFunc))
    return 0;

  if(!checkLocation(instance))
    return 0;

  reapack = API::reapack = new ReaPack(instance);

  setupActions();
  setupAPI();

  plugin_register("hookcommand", (void *)commandHook);
  plugin_register("hookcustommenu", (void *)menuHook);

  AddExtensionsMainMenu();

  return 1;
}

#ifndef _WIN32
#include "resource.hpp"

#include <swell/swell-dlggen.h>
#include "resource.rc_mac_dlg"

#include <swell/swell-menugen.h>
#include "resource.rc_mac_menu"
#endif
