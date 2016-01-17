/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2016  Christian Fillion
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

#include "errors.hpp"
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

  Menu menu = Menu(handle).addMenu(AUTO_STR("ReaPack"));

  menu.addAction(AUTO_STR("Synchronize packages"),
    NamedCommandLookup("_REAPACK_SYNC"));

  menu.addAction(AUTO_STR("Import remote repository..."),
    NamedCommandLookup("_REAPACK_IMPORT"));

  menu.addAction(AUTO_STR("Manage remotes..."),
    NamedCommandLookup("_REAPACK_MANAGE"));
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

  reapack.setupAction("REAPACK_MANAGE",
    bind(&ReaPack::manageRemotes, reapack));

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
