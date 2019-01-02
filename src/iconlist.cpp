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

#include "iconlist.hpp"

#ifdef _WIN32
#  define DEFINE_ICON(icon, rc, _) IconList::Icon IconList::icon = MAKEINTRESOURCE(rc)
#else
#  define DEFINE_ICON(icon, _, name) IconList::Icon IconList::icon = name
#endif

DEFINE_ICON(CheckedIcon, 141, "checked");
DEFINE_ICON(UncheckedIcon, 142, "unchecke");

IconList::IconList(const std::initializer_list<const Win32::char_type *> &icons)
{
  m_list = ImageList_Create(16, 16, 1, (int)icons.size(), (int)icons.size());

  for(const auto *icon : icons)
    loadIcon(icon);
}

void IconList::loadIcon(const Win32::char_type *name)
{
#ifdef _WIN32
  HINSTANCE reaper = GetModuleHandle(nullptr);
  HICON icon = LoadIcon(reaper, name);
  ImageList_AddIcon(m_list, icon);
#else
  HICON icon = LoadNamedImage(name, true);
  ImageList_Add(m_list, icon, 0);
#endif
  DestroyIcon(icon);
}

IconList::~IconList()
{
  ImageList_Destroy(m_list);
}
