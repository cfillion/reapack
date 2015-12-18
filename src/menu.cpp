/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015  Christian Fillion
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

#include "menu.hpp"

#ifdef _WIN32
#define MENUITEMINFO MENUITEMINFOA
#undef InsertMenuItem
#define InsertMenuItem InsertMenuItemA
#endif

Menu::Menu(HMENU handle)
  : m_handle(handle)
{
  m_size = GetMenuItemCount(m_handle);

  if(!empty())
    addSeparator();
}

void Menu::addAction(const char *label, const int commandId)
{
  MENUITEMINFO mii{};
  mii.cbSize = sizeof(MENUITEMINFO);

  mii.fMask |= MIIM_TYPE;
  mii.fType = MFT_STRING;
  mii.dwTypeData = const_cast<char *>(label);

  mii.fMask |= MIIM_ID;
  mii.wID = commandId;

  append(mii);
}

void Menu::addSeparator()
{
  MENUITEMINFO mii{};
  mii.cbSize = sizeof(MENUITEMINFO);

  mii.fMask = MIIM_TYPE;
  mii.fType = MFT_SEPARATOR;

  append(mii);
}

Menu Menu::addMenu(const char *label)
{
  MENUITEMINFO mii{};
  mii.cbSize = sizeof(MENUITEMINFO);

  mii.fMask |= MIIM_TYPE;
  mii.fType = MFT_STRING;
  mii.dwTypeData = const_cast<char *>(label);

  mii.fMask |= MIIM_SUBMENU;
  mii.hSubMenu = CreatePopupMenu();

  append(mii);

  return Menu(mii.hSubMenu);
}

void Menu::append(MENUITEMINFO &mii)
{
  InsertMenuItem(m_handle, m_size++, true, &mii);
}
