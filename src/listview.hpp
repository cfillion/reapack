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

#ifndef REAPACK_LISTVIEW_HPP
#define REAPACK_LISTVIEW_HPP

#include "control.hpp"

#include <boost/optional.hpp>
#include <boost/signals2.hpp>
#include <functional>
#include <vector>

#include "serializer.hpp"

class Menu;

class ListView : public Control {
public:
  enum SortOrder { AscendingOrder, DescendingOrder };

  enum ColumnFlag {
    NoLabelFlag  = 1<<0,
    CollapseFlag = 1<<1,
  };

  enum ColumnDataType {
    UserType,
    VersionType,
    TimeType,
  };

  struct Cell {
    Cell() : userData(nullptr) {}
    Cell(const Cell &) = delete;

    std::string value;
    void *userData;
  };

  class Row {
  public:
    Row(size_t size, void *data, ListView *);
    Row(const Row &) = delete;
    ~Row() { delete[] m_cells; }

    void *userData;

    int index() const { return m_userIndex; }

    const Cell &cell(const int i) const { return m_cells[i]; }
    void setCell(const int i, const std::string &, void *data = nullptr);

  protected:
    friend ListView;
    int viewIndex;

  private:
    int m_userIndex;
    ListView *m_list;
    Cell *m_cells;
  };

  typedef std::shared_ptr<Row> RowPtr;

  struct Column {
    std::string label;
    int width;
    int flags;
    ColumnDataType dataType;

    bool test(ColumnFlag f) const { return (flags & f) != 0; }
    int compare(const Cell &, const Cell &) const;
  };

  typedef std::vector<Column> Columns;

  typedef boost::signals2::signal<void ()> VoidSignal;
  typedef boost::signals2::signal<bool (Menu &, int index)> MenuSignal;

  ListView(HWND handle, const Columns & = {});

  void reserveRows(size_t count) { m_rows.reserve(count); }
  RowPtr createRow(void *data = nullptr);
  const RowPtr &row(size_t index) const { return m_rows[index]; }
  void updateCell(int row, int cell);
  void removeRow(int index);
  int rowCount() const { return (int)m_rows.size(); }
  bool empty() const { return m_rows.empty(); }

  void clear();
  void reset();
  void autoSizeHeader();

  int currentIndex() const;
  int itemUnderMouse() const;

  int scroll() const;
  void setScroll(int);

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
  int sortColumn() const { return m_sort ? m_sort->column : -1; }

  void restoreState(Serializer::Data &);
  void saveState(Serializer::Data &) const;
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

  bool m_customizable;
  std::vector<Column> m_cols;
  std::vector<RowPtr> m_rows;
  boost::optional<Sort> m_sort;
  boost::optional<Sort> m_defaultSort;

  VoidSignal m_onSelect;
  VoidSignal m_onActivate;
  MenuSignal m_onContextMenu;
};

#endif
