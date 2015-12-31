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

#ifndef REAPACK_LISTVIEW_HPP
#define REAPACK_LISTVIEW_HPP

#ifdef _WIN32
#include <windows.h>
#else
#include <swell/swell.h>
#endif

#include <boost/signals2.hpp>
#include <vector>

#include "encoding.hpp"

class ListView {
public:
  typedef std::pair<const auto_char *, int> Column;
  typedef std::vector<const Column> Columns;
  typedef std::vector<const auto_char *> Row;

  typedef boost::signals2::signal<void ()> Signal;
  typedef Signal::slot_type Callback;

  ListView(const Columns &, HWND handle);

  HWND handle() const { return m_handle; }

  void addRow(const Row &);
  void clear();

  int selectedIndex() const;

  void onSelect(const Callback &callback) { m_onSelect.connect(callback); }
  void onNotify(LPNMHDR, LPARAM);

private:
  void addColumn(const auto_char *text, const int width);

  HWND m_handle;
  size_t m_columnSize;
  size_t m_rowSize;

  Signal m_onSelect;
};

#endif
