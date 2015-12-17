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

#ifndef REAPACK_MENU_HPP
#define REAPACK_MENU_HPP

#ifdef _WIN32
#include <windows.h>
#else
#include <swell/swell.h>
#endif

class Menu {
public:
  Menu(HMENU handle);

  unsigned int size() { return m_size; }
  bool empty() const { return m_size == 0; }

  void addAction(const char *label, const int commandId);
  void addSeparator();
  Menu addMenu(const char *label);

private:
  void append(MENUITEMINFO &);

  HMENU m_handle;
  unsigned int m_size;
};

#endif
