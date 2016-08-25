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

#include "listview.hpp"

#include "menu.hpp"

#include <boost/algorithm/string.hpp>

#ifdef _WIN32
#include <commctrl.h>
#endif

using namespace std;

ListView::ListView(HWND handle, const Columns &columns)
  : Control(handle), m_customizable(false), m_sort(), m_defaultSort()
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
  LVCOLUMN item{};

  item.mask |= LVCF_WIDTH;
  item.cx = col.test(CollapseFlag) ? 0 : adjustWidth(col.width);

  if(!col.test(NoLabelFlag)) {
    item.mask |= LVCF_TEXT;
    item.pszText = const_cast<auto_char *>(col.label.c_str());
  }

  const int index = columnCount();
  ListView_InsertColumn(handle(), index, &item);
  m_cols.push_back(col);

  if(m_sort && m_sort->column == index)
    setSortArrow(true);

  return index;
}

int ListView::addRow(const Row &content)
{
  LVITEM item{};
  item.iItem = rowCount();

  item.mask |= LVIF_PARAM;
  item.lParam = item.iItem;

  ListView_InsertItem(handle(), &item);

  m_rows.resize(item.iItem + 1); // make room for the new row
  replaceRow(item.iItem, content);

  return item.iItem;
}

void ListView::replaceRow(int index, const Row &content)
{
  m_rows[index] = content;

  const int cols = min(columnCount(), (int)content.size());
  index = translate(index);

  for(int i = 0; i < cols; i++) {
    auto_char *text = const_cast<auto_char *>(content[i].c_str());
    ListView_SetItemText(handle(), index, i, text);
  }
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
  if(!m_sort)
    return;

  static const auto compare = [](LPARAM aRow, LPARAM bRow, LPARAM param)
  {
    ListView *view = reinterpret_cast<ListView *>(param);
    const int column = view->m_sort->column;

    int ret;

    const auto it = view->m_sortFuncs.find(column);
    if(it != view->m_sortFuncs.end())
      ret = it->second((int)aRow, (int)bRow);
    else {
      auto_string a = view->m_rows[aRow][column];
      boost::algorithm::to_lower(a);

      auto_string b = view->m_rows[bRow][column];
      boost::algorithm::to_lower(b);

      ret = a.compare(b);
    }

    switch(view->m_sort->order) {
    case AscendingOrder:
      return ret;
    case DescendingOrder:
    default: // for MSVC
      return -ret;
    }
  };

  ListView_SortItems(handle(), compare, (LPARAM)this);
}

void ListView::sortByColumn(const int index, const SortOrder order, const bool user)
{
  if(m_sort)
    setSortArrow(false);

  const auto settings = Sort(index, order);

  if(!user)
    m_defaultSort = settings;

  m_sort = settings;

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
  m_sortFuncs.clear();
}

void ListView::setSelected(const int index, const bool select)
{
  ListView_SetItemState(handle(), translate(index),
    select ? LVIS_SELECTED : 0, LVIS_SELECTED);
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
  vector<int> indexes;

  while((index = ListView_GetNextItem(handle(), index, LVNI_SELECTED)) != -1) {
    indexes.push_back(translateBack(index));
  }

  if(sort)
    std::sort(indexes.begin(), indexes.end());

  return indexes;
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
  sort();
}

int ListView::translate(const int userIndex) const
{
  if(!m_sort || userIndex < 0)
    return userIndex;

  for(int viewIndex = 0; viewIndex < rowCount(); viewIndex++) {
    LVITEM item{};
    item.iItem = viewIndex;
    item.mask |= LVIF_PARAM;
    ListView_GetItem(handle(), &item);

    if(item.lParam == userIndex)
      return viewIndex;
  }

  return -1;
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
  menu.addAction(AUTO_STR("Reset columns"), ACTION_RESTORE);

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

  if(m_sort && m_defaultSort) {
    setSortArrow(false);
    m_sort = m_defaultSort;
    setSortArrow(true);
    sort();
  }
}

void ListView::restore(Serializer::Data &data)
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
      if(left < columnCount())
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

void ListView::save(Serializer::Data &data) const
{
  const Sort sort = m_sort.value_or(Sort());
  vector<int> order(columnCount());
  ListView_GetColumnOrderArray(handle(), columnCount(), &order[0]);

  data.push_back({sort.column, sort.order});

  for(int i = 0; i < columnCount(); i++)
    data.push_back({order[i], columnWidth(i)});
}
