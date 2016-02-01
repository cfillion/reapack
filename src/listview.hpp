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

#ifndef REAPACK_LISTVIEW_HPP
#define REAPACK_LISTVIEW_HPP

#include "control.hpp"

#include <boost/signals2.hpp>
#include <vector>

#include "encoding.hpp"

class ListView : public Control {
public:
  struct Column { auto_string text; int width; };
  typedef std::vector<Column> Columns;
  typedef std::vector<auto_string> Row;

  typedef boost::signals2::signal<void ()> Signal;
  typedef Signal::slot_type Callback;

  ListView(const Columns &, HWND handle);

  int addRow(const Row &);
  Row getRow(const int index) const;
  void replaceRow(const int index, const Row &);
  void removeRow(const int index);
  void clear();

  bool hasSelection() const;
  int currentIndex() const;

  void onSelect(const Callback &callback) { m_onSelect.connect(callback); }
  void onDoubleClick(const Callback &callback)
  { m_onDoubleClick.connect(callback); }

protected:
  void onNotify(LPNMHDR, LPARAM) override;

private:
  void addColumn(const Column &);

  int m_columnSize;
  int m_rowSize;

  Signal m_onSelect;
  Signal m_onDoubleClick;
};

#endif
