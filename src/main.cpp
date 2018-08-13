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

#define REQUIRED_API(name) {reinterpret_cast<void **>(&name), #name, true}
#define OPTIONAL_API(name) {reinterpret_cast<void **>(&name), #name, false}

static bool loadAPI(void *(*getFunc)(const char *))
{
  struct ApiFunc { void **ptr; const char *name; bool required; };

  const ApiFunc funcs[] = {
    REQUIRED_API(Splash_GetWnd),             // v4.7

    REQUIRED_API(AddExtensionsMainMenu),
    REQUIRED_API(GetAppVersion),
    REQUIRED_API(GetResourcePath),
    REQUIRED_API(NamedCommandLookup),        // v3.1415
    REQUIRED_API(plugin_register),
    REQUIRED_API(ShowMessageBox),

    OPTIONAL_API(AddRemoveReaScript),        // v5.12
  };

  for(const ApiFunc &func : funcs) {
    *func.ptr = getFunc(func.name);

    if(func.required && *func.ptr == nullptr) {
      Win32::messageBox(Splash_GetWnd ? Splash_GetWnd() : nullptr, String::format(
        "ReaPack v%s is incompatible with this version of REAPER.\n\n"
        "(Unable to import the following API function: %s)",
        ReaPack::VERSION, func.name
      ).c_str(), "ReaPack: Missing REAPER feature", MB_OK);

      return false;
    }
  }

  return true;
}

static bool commandHook(const int id, const int flag)
{
  return g_reapack->actions()->run(id);
}

static void menuHook(const char *name, HMENU handle, const int f)
{
  if(strcmp(name, "Main extensions") || f != 0)
    return;

  Menu menu = Menu(handle).addMenu("ReaPack");
  menu.addAction("&Synchronize packages",   "_REAPACK_SYNC");
  menu.addAction("&Browse packages...",     "_REAPACK_BROWSE");
  menu.addAction("&Import repositories...", "_REAPACK_IMPORT");
  menu.addAction("&Manage repositories...", "_REAPACK_MANAGE");
  menu.addSeparator();
  menu.addAction(String::format("&About ReaPack v%s", ReaPack::VERSION), "_REAPACK_ABOUT");
}

static bool checkLocation(REAPER_PLUGIN_HINSTANCE module)
{
  Path expected;
  expected.append(ReaPack::resourcePath());
  expected.append("UserPlugins");
  expected.append(REAPACK_FILE);

#ifdef _WIN32
  Win32::char_type self[MAX_PATH]{};
  GetModuleFileName(module, self, static_cast<DWORD>(std::size(self)));
  Path current(Win32::narrow(self));
#else
  Dl_info info{};
  dladdr(reinterpret_cast<const void *>(&checkLocation), &info);
  Path current(info.dli_fname);
#endif

  if(current == expected)
    return true;

  Win32::messageBox(Splash_GetWnd(), String::format(
    "ReaPack was not loaded from the standard extension path"
    " or its filename was altered.\n"
    "Move or rename it to the expected location and retry.\n\n"
    "Current: %s\n\nExpected: %s",
    current.join().c_str(), expected.join().c_str()
  ).c_str(), "ReaPack: Installation path mismatch", MB_OK);

  return false;
}

extern "C" REAPER_PLUGIN_DLL_EXPORT int REAPER_PLUGIN_ENTRYPOINT(
  REAPER_PLUGIN_HINSTANCE instance, reaper_plugin_info_t *rec)
{
  if(!rec) {
    plugin_register("-hookcommand",    reinterpret_cast<void *>(commandHook));
    plugin_register("-hookcustommenu", reinterpret_cast<void *>(menuHook));

    delete g_reapack;

    return 0;
  }
  else if(rec->caller_version != REAPER_PLUGIN_VERSION
      || !loadAPI(rec->GetFunc) || !checkLocation(instance))
    return 0;

  new ReaPack(instance, rec->hwnd_main);

  plugin_register("hookcommand",    reinterpret_cast<void *>(commandHook));
  plugin_register("hookcustommenu", reinterpret_cast<void *>(menuHook));

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
