/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2021  Christian Fillion
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

#ifndef REAPACK_ICONLIST_HPP
#define REAPACK_ICONLIST_HPP

#include "win32.hpp"

#include <initializer_list>

#ifdef _WIN32
#  include <windows.h>
#  include <commctrl.h>
#else
#  include <swell/swell.h>
#endif

class IconList {
public:
  typedef const Win32::char_type *Icon;

  static Icon CheckedIcon;
  static Icon UncheckedIcon;

  IconList(const std::initializer_list<Icon> &icons);
  ~IconList();

  void loadIcon(Icon icon);
  HIMAGELIST handle() const { return m_list; }

private:
  HIMAGELIST m_list;
};

#endif
