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

#include <boost/optional.hpp>
#include <boost/signals2.hpp>
#include <vector>

#include "encoding.hpp"

class Menu;

class ListView : public Control {
public:
  enum SortOrder { AscendingOrder, DescendingOrder };

  struct Column { auto_string text; int width; };
  typedef std::vector<Column> Columns;
  typedef std::vector<auto_string> Row;

  typedef boost::signals2::signal<void ()> VoidSignal;
  typedef boost::signals2::signal<bool (Menu &, int index)> MenuSignal;

  ListView(const Columns &, HWND handle);

  int addRow(const Row &);
  const Row &row(int index) const { return m_rows[index]; }
  void replaceRow(int index, const Row &);
  void removeRow(int index);
  void resizeColumn(int index, int width);
  int columnSize(int index) const;
  void sort();
  void sortByColumn(int index, SortOrder order = AscendingOrder);
  void clear();
  void setSelected(int index, bool select);
  void select(int index) { setSelected(index, true); }
  void unselect(int index) { setSelected(index, false); }
  void selectAll() { select(-1); }
  void unselectAll() { unselect(-1); }

  int selectionSize() const;
  bool hasSelection() const { return selectionSize() > 0; }
  int currentIndex() const;
  std::vector<int> selection(bool sort = true) const;
  int itemUnderMouse() const;
  int rowCount() const { return (int)m_rows.size(); }
  int columnCount() const { return (int)m_cols.size(); }
  bool empty() const { return rowCount() < 1; }

  bool restore(const std::string &, int userVersion);
  void restoreDefaults();
  std::string save() const;

  void onSelect(const VoidSignal::slot_type &slot) { m_onSelect.connect(slot); }
  void onActivate(const VoidSignal::slot_type &slot) { m_onActivate.connect(slot); }
  void onContextMenu(const MenuSignal::slot_type &slot) { m_onContextMenu.connect(slot); }

protected:
  void onNotify(LPNMHDR, LPARAM) override;
  bool onContextMenu(HWND, int, int) override;

private:
  struct Sort {
    Sort(int c = -1, SortOrder o = AscendingOrder)
      : column(c), order(o) {}

    int column;
    SortOrder order;
  };

  static int adjustWidth(int);
  void setExStyle(int style, bool enable);
  void addColumn(const Column &);
  void setSortArrow(bool);
  void handleDoubleClick();
  void handleColumnClick(LPARAM lpnmlistview);
  int translate(int userIndex) const;
  int translateBack(int internalIndex) const;
  void headerMenu(int x, int y);

  int m_userVersion;
  std::vector<Column> m_cols;
  std::vector<Row> m_rows;
  boost::optional<Sort> m_sort;
  boost::optional<Sort> m_defaultSort;

  VoidSignal m_onSelect;
  VoidSignal m_onActivate;
  MenuSignal m_onContextMenu;
};

#endif
