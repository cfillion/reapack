/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2019  Christian Fillion
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

#include "filedialog.hpp"

#include "path.hpp"
#include "win32.hpp"

#ifndef _WIN32
#  include <swell.h>
#endif

using namespace std;

#ifdef _WIN32
static string getFileName(BOOL(__stdcall *func)(LPOPENFILENAME),
  HWND parent, HINSTANCE instance, const char *title, const Path &initialDir,
  const Win32::char_type *filters, const Win32::char_type *defaultExt)
{
  const auto &&wideTitle = Win32::widen(title);
  const auto &&wideInitialDir = Win32::widen(initialDir.join());

  wchar_t pathBuffer[4096] = {};

  OPENFILENAME of{sizeof(OPENFILENAME), parent, instance};
  of.lpstrFilter = filters;
  of.lpstrFile = pathBuffer;
  of.nMaxFile = static_cast<DWORD>(size(pathBuffer));
  of.lpstrInitialDir = wideInitialDir.c_str();
  of.lpstrTitle = wideTitle.c_str();
  of.Flags = OFN_HIDEREADONLY | OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT;
  of.lpstrDefExt = defaultExt;

  return func(&of) ? Win32::narrow(pathBuffer) : string();
}
#endif

string FileDialog::getOpenFileName(HWND parent, HINSTANCE instance,
  const char *title, const Path &initialDir,
  const Win32::char_type *filters, const Win32::char_type *defaultExt)
{
#ifdef _WIN32
  return getFileName(&GetOpenFileName, parent, instance,
    title, initialDir, filters, defaultExt);
#else
  const char *path = BrowseForFiles(title, initialDir.join().c_str(),
    nullptr, false, filters);
  return path ? path : string();
#endif
}

string FileDialog::getSaveFileName(HWND parent, HINSTANCE instance,
  const char *title, const Path &initialDir,
  const Win32::char_type *filters, const Win32::char_type *defaultExt)
{
#ifdef _WIN32
  return getFileName(&GetSaveFileName, parent, instance,
    title, initialDir, filters, defaultExt);
#else
  char path[4096] = {};

  if(BrowseForSaveFile(title, initialDir.join().c_str(),
      nullptr, filters, path, sizeof(path)))
    return path;
  else
    return {};
#endif
}
