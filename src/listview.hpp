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
#include <functional>
#include <vector>

#include "encoding.hpp"
#include "serializer.hpp"

class Menu;

class ListView : public Control {
public:
  enum SortOrder { AscendingOrder, DescendingOrder };

  enum ColumnFlag {
    NoLabelFlag  = 1<<0,
    CollapseFlag = 1<<1,
  };

  struct Column {
    auto_string label;
    int width;
    int flags;

    bool test(ColumnFlag f) const { return (flags & f) != 0; }
  };

  typedef std::vector<Column> Columns;
  typedef std::vector<auto_string> Row;

  typedef boost::signals2::signal<void ()> VoidSignal;
  typedef boost::signals2::signal<bool (Menu &, int index)> MenuSignal;
  typedef std::function<int (int, int)> SortCallback;

  ListView(const Columns &, HWND handle);

  int addRow(const Row &);
  const Row &row(int index) const { return m_rows[index]; }
  void replaceRow(int index, const Row &);
  void removeRow(int index);
  int rowCount() const { return (int)m_rows.size(); }
  bool empty() const { return rowCount() < 1; }
  void clear();
  int currentIndex() const;
  int itemUnderMouse() const;

  void setSelected(int index, bool select);
  void select(int index) { setSelected(index, true); }
  void unselect(int index) { setSelected(index, false); }
  void selectAll() { select(-1); }
  void unselectAll() { unselect(-1); }
  int selectionSize() const;
  bool hasSelection() const { return selectionSize() > 0; }
  std::vector<int> selection(bool sort = true) const;

  int addColumn(const Column &);
  void resizeColumn(int index, int width);
  int columnWidth(int index) const;
  int columnCount() const { return (int)m_cols.size(); }

  void sort();
  void sortByColumn(int index, SortOrder order = AscendingOrder, bool user = false);
  void setSortCallback(int i, const SortCallback &cb) { m_sortFuncs[i] = cb; }

  void restore(const std::string &, int userVersion);
  std::string save() const;
  void resetColumns();

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
  void setSortArrow(bool);
  void handleDoubleClick();
  void handleColumnClick(LPARAM lpnmlistview);
  int translate(int userIndex) const;
  int translateBack(int internalIndex) const;
  void headerMenu(int x, int y);

  Serializer m_serializer;
  std::vector<Column> m_cols;
  std::vector<Row> m_rows;
  boost::optional<Sort> m_sort;
  boost::optional<Sort> m_defaultSort;
  std::map<int, SortCallback> m_sortFuncs;

  VoidSignal m_onSelect;
  VoidSignal m_onActivate;
  MenuSignal m_onContextMenu;
};

#endif
