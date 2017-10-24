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

#include "listview.hpp"

#include "menu.hpp"
#include "time.hpp"
#include "version.hpp"
#include "win32.hpp"

#include <boost/algorithm/string/case_conv.hpp>

#ifdef _WIN32
#  include <commctrl.h>
#endif

using namespace std;

ListView::ListView(HWND handle, const Columns &columns)
  : Control(handle), m_customizable(false), m_sort(), m_defaultSort(), m_dirty(0)
{
  for(const Column &col : columns)
    addColumn(col);

  setExStyle(LVS_EX_FULLROWSELECT, true);

#ifdef LVS_EX_LABELTIP
  // unsupported by SWELL, but always enabled on OS X anyway
  setExStyle(LVS_EX_LABELTIP, true);
#endif
}

void ListView::setExStyle(const int style, const bool enable)
{
  ListView_SetExtendedListViewStyleEx(handle(), style, enable ? style : 0);
}

int ListView::addColumn(const Column &col)
{
  assert(m_rows.empty());

  LVCOLUMN item{};

  item.mask |= LVCF_WIDTH;
  item.cx = col.test(CollapseFlag) ? 0 : adjustWidth(col.width);

  const auto &&label = Win32::widen(col.label);
  if(!col.test(NoLabelFlag)) {
    item.mask |= LVCF_TEXT;
    item.pszText = const_cast<Win32::char_type *>(label.c_str());
  }

  const int index = columnCount();
  ListView_InsertColumn(handle(), index, &item);
  m_cols.push_back(col);

  if(m_sort && m_sort->column == index)
    setSortArrow(true);

  return index;
}

auto ListView::createRow(void *data) -> RowPtr
{
  const int index = rowCount();
  insertItem(index, index);

  RowPtr row = make_shared<Row>(data, this);
  m_rows.push_back(row);

  return row;
}

void ListView::insertItem(const int viewIndex, const int rowIndex)
{
  LVITEM item{};
  item.iItem = viewIndex;

  item.mask |= LVIF_PARAM;
  item.lParam = rowIndex;

  ListView_InsertItem(handle(), &item);
}

void ListView::updateCell(int row, int cell)
{
  const int viewRowIndex = translate(row);
  const auto &&text = Win32::widen(m_rows[row]->cell(cell).value);

  ListView_SetItemText(handle(), viewRowIndex, cell,
    const_cast<Win32::char_type *>(text.c_str()));

  if(m_sort && m_sort->column == cell)
    m_dirty |= NeedSortFlag;

  m_dirty |= NeedFilterFlag;
}

void ListView::removeRow(const int userIndex)
{
  // translate to view index before fixing lParams
  const int viewIndex = translate(userIndex);

  // shift lParam to reflect the new row indexes
  map<int, int> translations;
  const int size = rowCount();
  for(int i = userIndex + 1; i < size; i++)
    translations[translate(i)] = i - 1;

  for(const auto &it : translations) {
    LVITEM item{};
    item.iItem = it.first;
    item.mask |= LVIF_PARAM;
    item.lParam = it.second;
    ListView_SetItem(handle(), &item);
  }

  ListView_DeleteItem(handle(), viewIndex);
  m_rows.erase(m_rows.begin() + userIndex);
}

void ListView::resizeColumn(const int index, const int width)
{
  ListView_SetColumnWidth(handle(), index, adjustWidth(width));
}

int ListView::columnWidth(const int index) const
{
  return ListView_GetColumnWidth(handle(), index);
}

void ListView::sort()
{
  static const auto compare = [](LPARAM aRow, LPARAM bRow, LPARAM param)
  {
    const int indexDiff = (int)(aRow - bRow);

    ListView *view = reinterpret_cast<ListView *>(param);

    if(!view->m_sort)
      return indexDiff;

    const int columnIndex = view->m_sort->column;
    const Column &column = view->m_cols[columnIndex];

    int ret = column.compare(view->row(aRow)->cell(columnIndex),
      view->row(bRow)->cell(columnIndex));

    if(view->m_sort->order == DescendingOrder)
      ret = -ret;

    return ret ? ret : indexDiff;
  };

  ListView_SortItems(handle(), compare, (LPARAM)this);

  m_dirty = (m_dirty | NeedReindexFlag) & ~NeedSortFlag;
}

void ListView::sortByColumn(const int index, const SortOrder order, const bool user)
{
  if(m_sort)
    setSortArrow(false);

  const Sort settings(index, order);

  if(!user)
    m_defaultSort = settings;

  m_sort = settings;
  m_dirty |= NeedSortFlag;

  setSortArrow(true);
}

void ListView::setSortArrow(const bool set)
{
  if(!m_sort)
    return;

  HWND header = ListView_GetHeader(handle());

  HDITEM item{};
  item.mask |= HDI_FORMAT;

  if(!Header_GetItem(header, m_sort->column, &item))
    return;

  item.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP); // clear

  if(set) {
    switch(m_sort->order) {
    case AscendingOrder:
      item.fmt |= HDF_SORTUP;
      break;
    case DescendingOrder:
      item.fmt |= HDF_SORTDOWN;
    }
  }

  Header_SetItem(header, m_sort->column, &item);
}

void ListView::filter()
{
  vector<int> hide;

  for(int ri = 0; ri < rowCount(); ++ri) {
    RowPtr &row = m_rows[ri];

    if(m_filter.match(row->filterValues())) {
      if(row->viewIndex == -1) {
        row->viewIndex = visibleRowCount();
        insertItem(row->viewIndex, ri);

        for(int ci = 0; ci < columnCount(); ++ci)
          updateCell(ri, ci);

        m_dirty |= NeedSortFlag;
      }
    }
    else if(row->viewIndex > -1) {
      hide.emplace_back(row->viewIndex);
      row->viewIndex = -1;
    }
  }

  std::sort(hide.begin(), hide.end());
  for(int i = 0; i < (int)hide.size(); ++i) {
    ListView_DeleteItem(handle(), hide[i] - i);
    m_dirty |= NeedReindexFlag;
  }

  m_dirty &= ~NeedFilterFlag;
}

void ListView::setFilter(const string &newFilter)
{
  if(m_filter != newFilter) {
    ListView::BeginEdit edit(this);
    m_filter = newFilter;
    m_dirty |= NeedFilterFlag;
  }
}

void ListView::reindexVisible()
{
  const int visibleCount = visibleRowCount();
  for(int viewIndex = 0; viewIndex < visibleCount; viewIndex++) {
    LVITEM item{};
    item.iItem = viewIndex;
    item.mask |= LVIF_PARAM;
    ListView_GetItem(handle(), &item);

    row(item.lParam)->viewIndex = viewIndex;
  }

  m_dirty &= ~NeedReindexFlag;
}

void ListView::endEdit()
{
  if(m_dirty & NeedFilterFlag)
    filter(); // filter may set NeedSortFlag
  if(m_dirty & NeedSortFlag)
    sort(); // sort may set NeedReindexFlag
  if(m_dirty & NeedReindexFlag)
    reindexVisible();

  assert(!m_dirty);
}

void ListView::clear()
{
  ListView_DeleteAllItems(handle());

  m_rows.clear();
}

void ListView::reset()
{
  clear();

  for(int i = columnCount(); i > 0; i--)
    ListView_DeleteColumn(handle(), i - 1);

  m_cols.clear();

  m_customizable = false;
  m_sort = boost::none;
  m_defaultSort = boost::none;
}

void ListView::setSelected(const int index, const bool select)
{
  ListView_SetItemState(handle(), translate(index),
    select ? LVIS_SELECTED : 0, LVIS_SELECTED);
}

int ListView::visibleRowCount() const
{
  return ListView_GetItemCount(handle());
}

int ListView::currentIndex() const
{
  const int internalIndex = ListView_GetNextItem(handle(), -1, LVNI_SELECTED);

  if(internalIndex < 0)
    return -1;
  else
    return translateBack(internalIndex);
}

vector<int> ListView::selection(const bool sort) const
{
  int index = -1;
  vector<int> selectedIndexes;

  while((index = ListView_GetNextItem(handle(), index, LVNI_SELECTED)) != -1)
    selectedIndexes.push_back(translateBack(index));

  if(sort)
    std::sort(selectedIndexes.begin(), selectedIndexes.end());

  return selectedIndexes;
}

int ListView::selectionSize() const
{
  return ListView_GetSelectedCount(handle());
}

int ListView::itemUnderMouse() const
{
  LVHITTESTINFO info{};
  GetCursorPos(&info.pt);
  ScreenToClient(handle(), &info.pt);
  ListView_HitTest(handle(), &info);

  return translateBack(info.iItem);
}

int ListView::scroll() const
{
  return ListView_GetTopIndex(handle());
}

void ListView::setScroll(const int index)
{
#ifdef ListView_GetItemPosition
  if(index < 0)
    return;

  RECT rect;
  ListView_GetViewRect(handle(), &rect);

  POINT itemPos{};
  if(ListView_GetItemPosition(handle(), index - 1, &itemPos))
    ListView_Scroll(handle(), abs(rect.left) + itemPos.x, abs(rect.top) + itemPos.y);
#endif
}

void ListView::autoSizeHeader()
{
#ifdef LVSCW_AUTOSIZE_USEHEADER
  resizeColumn(columnCount() - 1, LVSCW_AUTOSIZE_USEHEADER);
#endif
}

void ListView::onNotify(LPNMHDR info, LPARAM lParam)
{
  switch(info->code) {
  case LVN_ITEMCHANGED:
    m_onSelect();
    break;
  case NM_DBLCLK:
    handleDoubleClick();
    break;
  case LVN_COLUMNCLICK:
    handleColumnClick(lParam);
    break;
  };
}

bool ListView::onContextMenu(HWND dialog, int x, int y)
{
  SetFocus(handle());

  POINT point{x, y};
  ScreenToClient(handle(), &point);

#ifdef _WIN32
  HWND header = ListView_GetHeader(handle());
  RECT rect;
  GetWindowRect(header, &rect);
  const int headerHeight = rect.bottom - rect.top;
#else
  // point.y is negative on OS X when hovering the header
  const int headerHeight = 0;
#endif

  if(point.y < headerHeight && x != -1) {
    if(m_customizable) // show menu only if header is customizable
      headerMenu(x, y);
    return true;
  }

  int index = itemUnderMouse();

#ifdef ListView_GetItemPosition // unsuported by SWELL
  // adjust the context menu's position when using Shift+F10 on Windows
  if(x == -1) {
    index = currentIndex();

    // find the location of the current item or of the first item
    POINT itemPos{};
    ListView_GetItemPosition(handle(), translate(max(0, index)), &itemPos);
    ClientToScreen(handle(), &itemPos);

    RECT controlRect;
    GetWindowRect(handle(), &controlRect);

    x = max(controlRect.left, min(itemPos.x, controlRect.right));
    y = max(controlRect.top, min(itemPos.y, controlRect.bottom));
  }
#endif

  Menu menu;

  if(!m_onContextMenu(menu, index))
    return false;

  menu.show(x, y, dialog);

  return true;
}

void ListView::handleDoubleClick()
{
  // user double clicked on an item
  if(itemUnderMouse() > -1 && currentIndex() > -1)
    m_onActivate();
}

void ListView::handleColumnClick(LPARAM lParam)
{
  auto info = (LPNMLISTVIEW)lParam;
  const int col = info->iSubItem;
  SortOrder order = AscendingOrder;

  if(m_sort && col == m_sort->column) {
    switch(m_sort->order) {
    case AscendingOrder:
      order = DescendingOrder;
      break;
    case DescendingOrder:
      order = AscendingOrder;
      break;
    }
  }

  sortByColumn(col, order, true);
  endEdit();
}

int ListView::translate(const int userIndex) const
{
  if(!m_sort || userIndex < 0)
    return userIndex;
  else
    return row(userIndex)->viewIndex;
}

int ListView::translateBack(const int internalIndex) const
{
  if(!m_sort || internalIndex < 0)
    return internalIndex;

  LVITEM item{};
  item.iItem = internalIndex;
  item.mask |= LVIF_PARAM;

  if(ListView_GetItem(handle(), &item))
    return (int)item.lParam;
  else
    return -1;
}

int ListView::adjustWidth(const int points)
{
#ifdef _WIN32
  if(points < 1)
    return points;
  else
    return (int)ceil(points * 0.863); // magic number to make pretty sizes...
#else
  return points;
#endif
}

void ListView::headerMenu(const int x, const int y)
{
  enum { ACTION_RESTORE = 800 };

  Menu menu;

  for(int i = 0; i < columnCount(); i++) {
    const auto id = menu.addAction(m_cols[i].label.c_str(), i | (1 << 8));

    if(columnWidth(id))
      menu.check(id);
  }

  menu.addSeparator();
  menu.addAction("Reset columns", ACTION_RESTORE);

  const int id = menu.show(x, y, handle());

  if(id == ACTION_RESTORE)
    resetColumns();
  else if(id >> 8 == 1) {
    const int col = id & 0xff;
    resizeColumn(col, columnWidth(col) ? 0 : m_cols[col].width);
  }
}

void ListView::resetColumns()
{
  vector<int> order(columnCount());

  for(int i = 0; i < columnCount(); i++) {
    order[i] = i;

    const Column &col = m_cols[i];
    resizeColumn(i, col.test(CollapseFlag) ? 0 : col.width);
  }

  ListView_SetColumnOrderArray(handle(), columnCount(), &order[0]);

  if(m_sort) {
    setSortArrow(false);
    m_sort = m_defaultSort;
    setSortArrow(true);

    m_dirty |= NeedSortFlag;
    endEdit();
  }
}

void ListView::restoreState(Serializer::Data &data)
{
  m_customizable = true;
  setExStyle(LVS_EX_HEADERDRAGDROP, true); // enable column reordering

  if(data.empty())
    return;

  int col = -1;
  vector<int> order(columnCount());

  while(!data.empty()) {
    const auto &rec = data.front();

    const int left = rec[0];
    const int right = rec[1];

    switch(col) {
    case -1: // sort
      if(left >= 0 && left < columnCount())
        sortByColumn(left, right == 0 ? AscendingOrder : DescendingOrder, true);
      break;
    default: // column
      order[col] = left;
      // raw size should not go through adjustSize (via resizeColumn)
      ListView_SetColumnWidth(handle(), col, right);
      break;
    }

    data.pop_front();

    if(++col >= columnCount())
      break;
  }

  // finish filling for other columns
  for(col++; col < columnCount(); col++)
    order[col] = col;

  ListView_SetColumnOrderArray(handle(), columnCount(), &order[0]);
}

void ListView::saveState(Serializer::Data &data) const
{
  const Sort &sort = m_sort.value_or(Sort{});
  vector<int> order(columnCount());
  ListView_GetColumnOrderArray(handle(), columnCount(), &order[0]);

  data.push_back({sort.column, sort.order});

  for(int i = 0; i < columnCount(); i++)
    data.push_back({order[i], columnWidth(i)});
}

int ListView::Column::compare(const ListView::Cell &cl, const ListView::Cell &cr) const
{
  if(dataType) {
    if(!cl.userData)
      return -1;
    else if(!cr.userData)
      return 1;
  }

  switch(dataType) {
  case UserType: { // arbitrary data or no data: sort by visible text
    string l = cl.value, r = cr.value;

    boost::algorithm::to_lower(l);
    boost::algorithm::to_lower(r);

    return l.compare(r);
  }
  case VersionType:
    return static_cast<const VersionName *>(cl.userData)->compare(
      *static_cast<const VersionName *>(cr.userData));
  case TimeType:
    return static_cast<const Time *>(cl.userData)->compare(
      *static_cast<const Time *>(cr.userData));
  }

  return 0; // to make MSVC happy
}

ListView::Row::Row(void *data, ListView *list)
  : userData(data), viewIndex(list->rowCount()), m_userIndex(viewIndex),
  m_list(list), m_cells(new Cell[m_list->columnCount()])
{
}

void ListView::Row::setCell(const int i, const string &val, void *data)
{
  Cell &cell = m_cells[i];
  cell.value = val;
  cell.userData = data;

  m_list->updateCell(m_userIndex, i);
}

vector<string> ListView::Row::filterValues() const
{
  vector<string> values;

  for(int ci = 0; ci < m_list->columnCount(); ++ci) {
    if(m_list->column(ci).test(FilterFlag))
      values.push_back(m_cells[ci].value);
  }

  return values;
}
