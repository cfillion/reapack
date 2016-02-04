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
  enum SortOrder { AscendingOrder, DescendingOrder };

  struct Column { auto_string text; int width; };
  typedef std::vector<Column> Columns;
  typedef std::vector<auto_string> Row;

  typedef boost::signals2::signal<void ()> Signal;
  typedef Signal::slot_type Callback;

  ListView(const Columns &, HWND handle);

  int addRow(const Row &);
  const Row &row(int index) const { return m_rows[index]; }
  void replaceRow(int index, const Row &);
  void removeRow(int index);
  void resizeColumn(int index, int width);
  void sort();
  void sortByColumn(int index, SortOrder order = AscendingOrder);
  void clear();

  bool hasSelection() const;
  int currentIndex() const;
  int rowCount() const { return (int)m_rows.size(); }

  void onSelect(const Callback &callback) { m_onSelect.connect(callback); }
  void onActivate(const Callback &callback) { m_onActivate.connect(callback); }

protected:
  void onNotify(LPNMHDR, LPARAM) override;

private:
  void setExStyle(int style, bool enable);
  void addColumn(const Column &);
  void setSortArrow(bool);
  void handleDoubleClick();
  void handleColumnClick(LPARAM lpnmlistview);
  int translate(int index) const;

  int m_columnSize;
  int m_sortColumn;
  SortOrder m_sortOrder;
  std::vector<Row> m_rows;

  Signal m_onSelect;
  Signal m_onActivate;
};

#endif
