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

#include "menu.hpp"

#include "win32.hpp"

#ifndef MIIM_FTYPE // for SWELL
#  define MIIM_FTYPE MIIM_TYPE
#endif

#ifndef _WIN32
#  include <swell.h>
#endif

using namespace std;

Menu::Menu(HMENU handle)
  : m_handle(handle), m_ownership(!handle)
{
  if(m_ownership)
    m_handle = CreatePopupMenu();

  m_size = GetMenuItemCount(m_handle);

  if(!empty())
    addSeparator();
}

Menu::~Menu()
{
  if(m_ownership)
    DestroyMenu(m_handle);
}

UINT Menu::addAction(const string &label, const int commandId)
{
  MENUITEMINFO mii{};
  mii.cbSize = sizeof(MENUITEMINFO);

  mii.fMask |= MIIM_TYPE;
  mii.fType = MFT_STRING;
  const auto &&wideLabel = Win32::widen(label);
  mii.dwTypeData = const_cast<Win32::char_type *>(wideLabel.c_str());

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

Menu Menu::addMenu(const string &label)
{
  MENUITEMINFO mii{};
  mii.cbSize = sizeof(MENUITEMINFO);

  mii.fMask |= MIIM_TYPE;
  mii.fType = MFT_STRING;
  const auto &&wideLabel = Win32::widen(label);
  mii.dwTypeData = const_cast<Win32::char_type *>(wideLabel.c_str());

  mii.fMask |= MIIM_SUBMENU;
  mii.hSubMenu = CreatePopupMenu();

  append(mii);

  return Menu(mii.hSubMenu);
}

void Menu::append(MENUITEMINFO &mii)
{
  InsertMenuItem(m_handle, m_size++, true, &mii);
}

int Menu::show(const int x, const int y, HWND parent) const
{
  const int command = TrackPopupMenu(m_handle,
    TPM_TOPALIGN | TPM_LEFTALIGN | TPM_NONOTIFY | TPM_RETURNCMD,
    x, y, 0, parent, nullptr);

  // both send the notification and return the command id
  SendMessage(parent, WM_COMMAND, command, 0);

  return command;
}

int Menu::show(HWND control, HWND parent) const
{
  RECT rect;
  GetWindowRect(control, &rect);

  return show(rect.left, rect.bottom - 1, parent);
}

void Menu::disableAll()
{
  for(UINT i = 0; i < m_size; i++)
    setEnabled(false, i);
}

void Menu::disable(const UINT index)
{
  setEnabled(false, index);
}

void Menu::enableAll()
{
  for(UINT i = 0; i < m_size; i++)
    setEnabled(false, i);
}

void Menu::enable(const UINT index)
{
  setEnabled(true, index);
}

void Menu::setEnabled(const bool enabled, const UINT index)
{
  MENUITEMINFO mii{};
  mii.cbSize = sizeof(MENUITEMINFO);

  if(!GetMenuItemInfo(m_handle, index, true, &mii))
    return;

  mii.fMask |= MIIM_STATE;
  mii.fState |= enabled ? MFS_ENABLED : MFS_DISABLED;

  SetMenuItemInfo(m_handle, index, true, &mii);
}

void Menu::check(const UINT index)
{
  MENUITEMINFO mii{};
  mii.cbSize = sizeof(MENUITEMINFO);
  mii.fMask |= MIIM_STATE;

  if(!GetMenuItemInfo(m_handle, index, true, &mii))
    return;

  mii.fState |= MFS_CHECKED;

  SetMenuItemInfo(m_handle, index, true, &mii);
}

void Menu::checkRadio(const UINT index)
{
  MENUITEMINFO mii{};
  mii.cbSize = sizeof(MENUITEMINFO);
  mii.fMask |= MIIM_FTYPE;
  mii.fMask |= MIIM_STATE;

  if(!GetMenuItemInfo(m_handle, index, true, &mii))
    return;

  mii.fType |= MFT_RADIOCHECK;
  mii.fState |= MFS_CHECKED;

  SetMenuItemInfo(m_handle, index, true, &mii);
}
