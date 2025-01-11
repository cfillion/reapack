/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2025  Christian Fillion
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

#include "event.hpp"
#include "filter.hpp"
#include "serializer.hpp"

#include <functional>
#include <optional>
#include <vector>

class Menu;

class ListView : public Control {
public:
  enum SortOrder { AscendingOrder, DescendingOrder };

  enum ColumnFlag {
    NoLabelFlag  = 1<<0,
    CollapseFlag = 1<<1,
    FilterFlag   = 1<<2,
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
    Row(void *data, ListView *);
    Row(const Row &) = delete;
    ~Row() { delete[] m_cells; }

    void *userData;

    int index() const { return userIndex; }

    const Cell &cell(const int i) const { return m_cells[i]; }
    void setCell(const int i, const std::string &, void *data = nullptr);
    void setChecked(bool check = true);

    std::vector<std::string> filterValues() const;

  protected:
    friend ListView;
    int viewIndex;
    int userIndex;

  private:
    ListView *m_list;
    Cell *m_cells;
  };

  struct Column {
    std::string label;
    int width;
    int flags;
    ColumnDataType dataType;

    bool test(ColumnFlag f) const { return (flags & f) != 0; }
    int compare(const Cell &, const Cell &) const;
  };

  // Use before modifying the list's content. It will re-sort and/or re-filter the
  // list automatically when destructed and, on Windows, also prevents flickering.
  class BeginEdit {
  public:
    BeginEdit(ListView *lv) : m_list(lv), m_inhibit(lv) {}
    ~BeginEdit() { m_list->endEdit(); }

  private:
    ListView *m_list;
    InhibitControl m_inhibit;
  };

  typedef std::vector<Column> Columns;

  ListView(HWND handle, const Columns & = {});

  void reserveRows(size_t count) { m_rows.reserve(count); }
  Row *createRow(void *data = nullptr);
  Row *row(size_t index) const { return m_rows[index].get(); }
  void removeRow(int index);
  int rowCount() const { return static_cast<int>(m_rows.size()); }
  int visibleRowCount() const;
  bool empty() const { return m_rows.empty(); }

  void clear();
  void reset();
  void autoSizeHeader();
  void enableIcons();

  int currentIndex() const;
  bool headerHitTest(int x, int y) const;
  int itemUnder(int x, int y, bool *overIcon = nullptr) const;
  int itemUnderMouse(bool *overIcon = nullptr) const;

  int scroll() const;
  void setScroll(int);

  void setSelected(int index, bool select);
  void select(int index) { setSelected(index, true); }
  void unselect(int index) { setSelected(index, false); }
  void selectAll();
  void unselectAll();
  int selectionSize() const;
  bool hasSelection() const { return selectionSize() > 0; }
  std::vector<int> selection(bool sort = true) const;

  int addColumn(const Column &);
  const Column &column(int index) const { return m_cols[index]; }
  void resizeColumn(int index, int width);
  int columnWidth(int index) const;
  int columnCount() const;

  void sortByColumn(int index, SortOrder order = AscendingOrder, bool user = false);
  void setFilter(const std::string &);
  void endEdit();

  void restoreState(Serializer::Data &);
  void saveState(Serializer::Data &) const;
  void resetColumns();

  Event<void()> onSelect;
  Event<void()> onIconClick;
  Event<void()> onActivate;
  Event<bool(Menu &, int index)> onFillContextMenu;

protected:
  friend Row;

  void updateCell(int row, int cell);
  void setRowIcon(int row, int icon);

private:
  struct Sort {
    int column = -1;
    SortOrder order = AscendingOrder;
  };

  enum DirtyFlag {
    NeedSortFlag    = 1<<0,
    NeedReindexFlag = 1<<1,
    NeedFilterFlag  = 1<<2,
  };

  void onNotify(LPNMHDR, LPARAM) override;
  bool onContextMenu(HWND, int, int) override;

  void setExStyle(int style, bool enable = true);
  void setSortArrow(bool);
  void onItemChanged(LPARAM lpnmlistview);
  void onClick(bool dbclick);
  void onColumnClick(LPARAM lpnmlistview);
  int translate(int userIndex) const;
  int translateBack(int internalIndex) const;
  void headerMenu(int x, int y);
  void insertItem(int viewIndex, int rowIndex);
  void sort();
  void reindexVisible();
  void filter();

  int m_dirty;
  Filter m_filter;

  bool m_customizable;
  std::vector<Column> m_cols;
  std::vector<std::unique_ptr<Row>> m_rows;
  std::optional<Sort> m_sort;
  std::optional<Sort> m_defaultSort;
};

#endif
