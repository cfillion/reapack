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

#include "menu.hpp"

Menu::Menu(HMENU handle)
  : m_handle(handle)
{
  if(!handle)
    m_handle = CreatePopupMenu();

  m_size = GetMenuItemCount(m_handle);

  if(!empty())
    addSeparator();
}

UINT Menu::addAction(const auto_char *label, const int commandId)
{
  MENUITEMINFO mii{};
  mii.cbSize = sizeof(MENUITEMINFO);

  mii.fMask |= MIIM_TYPE;
  mii.fType = MFT_STRING;
  mii.dwTypeData = const_cast<auto_char *>(label);

  mii.fMask |= MIIM_ID;
  mii.wID = commandId;

  const int index = m_size;
  append(mii);
  return index;
}

void Menu::addSeparator()
{
  MENUITEMINFO mii{};
  mii.cbSize = sizeof(MENUITEMINFO);

  mii.fMask = MIIM_TYPE;
  mii.fType = MFT_SEPARATOR;

  append(mii);
}

Menu Menu::addMenu(const auto_char *label)
{
  MENUITEMINFO mii{};
  mii.cbSize = sizeof(MENUITEMINFO);

  mii.fMask |= MIIM_TYPE;
  mii.fType = MFT_STRING;
  mii.dwTypeData = const_cast<auto_char *>(label);

  mii.fMask |= MIIM_SUBMENU;
  mii.hSubMenu = CreatePopupMenu();

  append(mii);

  return Menu(mii.hSubMenu);
}

void Menu::append(MENUITEMINFO &mii)
{
  InsertMenuItem(m_handle, m_size++, true, &mii);
}

void Menu::show(const int x, const int y, HWND parent) const
{
  TrackPopupMenu(m_handle, TPM_BOTTOMALIGN | TPM_LEFTALIGN,
    x, y, 0, parent, nullptr);
}

void Menu::disable()
{
  setEnabled(false);
}

void Menu::disable(const UINT index)
{
  setEnabled(false, index);
}

void Menu::enable()
{
  setEnabled(true);
}

void Menu::enable(const UINT index)
{
  setEnabled(true, index);
}

void Menu::setEnabled(const bool enabled)
{
  for(UINT i = 0; i < m_size; i++)
    setEnabled(enabled, i);
}

void Menu::setEnabled(const bool enabled, const UINT index)
{
  MENUITEMINFO mii{};
  mii.cbSize = sizeof(MENUITEMINFO);

  GetMenuItemInfo(m_handle, index, true, &mii);

  mii.fMask |= MIIM_STATE;
  mii.fState |= enabled ? MFS_ENABLED : MFS_DISABLED;

  SetMenuItemInfo(m_handle, index, true, &mii);
}
