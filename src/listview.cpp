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
#include <sstream>

#ifdef _WIN32
#include <commctrl.h>
#endif

using namespace std;

static const unsigned short VERSION = 1;
static const char FIELD_END = '\x20';
static const char RECORD_END = ',';

ListView::ListView(const Columns &columns, HWND handle)
  : Control(handle), m_userVersion(0), m_columnSize(0),
    m_sortColumn(-1), m_sortOrder(AscendingOrder)
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

void ListView::addColumn(const Column &col)
{
  LVCOLUMN item{};

  item.mask |= LVCF_WIDTH;
  item.cx = adjustWidth(col.width);

  item.mask |= LVCF_TEXT;
  item.pszText = const_cast<auto_char *>(col.text.c_str());

  ListView_InsertColumn(handle(), m_columnSize++, &item);
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

  const int cols = min(m_columnSize, (int)content.size());
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

int ListView::columnSize(const int index) const
{
  return ListView_GetColumnWidth(handle(), index);
}

void ListView::sort()
{
  if(m_sortColumn > -1)
    sortByColumn(m_sortColumn, m_sortOrder);
}

void ListView::sortByColumn(const int index, const SortOrder order)
{
  static const auto compare = [](LPARAM aRow, LPARAM bRow, LPARAM param)
  {
    ListView *view = reinterpret_cast<ListView *>(param);
    const int column = view->m_sortColumn;

    auto_string a = view->m_rows[aRow][column];
    boost::algorithm::to_lower(a);

    auto_string b = view->m_rows[bRow][column];
    boost::algorithm::to_lower(b);

    const int ret = a.compare(b);

    switch(view->m_sortOrder) {
    case AscendingOrder:
      return ret;
    case DescendingOrder:
    default: // for MSVC
      return -ret;
    }
  };

  if(m_sortColumn > -1)
    setSortArrow(false);

  m_sortColumn = index;
  m_sortOrder = order;
  setSortArrow(true);

  ListView_SortItems(handle(), compare, (LPARAM)this);
}

void ListView::setSortArrow(const bool set)
{
  HWND header = ListView_GetHeader(handle());

  HDITEM item{};
  item.mask |= HDI_FORMAT;

  if(!Header_GetItem(header, m_sortColumn, &item))
    return;

  item.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP); // clear

  if(set) {
    switch(m_sortOrder) {
    case AscendingOrder:
      item.fmt |= HDF_SORTUP;
      break;
    case DescendingOrder:
      item.fmt |= HDF_SORTDOWN;
    }
  }

  Header_SetItem(header, m_sortColumn, &item);
}

void ListView::clear()
{
  ListView_DeleteAllItems(handle());

  m_rows.clear();
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

  Menu menu;

  if(!m_onContextMenu(menu))
    return false;

  menu.show(x, y, dialog);
  return true;
}

void ListView::handleDoubleClick()
{
  const int index = itemUnderMouse();

  // user double clicked on an item
  if(index > -1)
    m_onActivate();
}

void ListView::handleColumnClick(LPARAM lParam)
{
  auto info = (LPNMLISTVIEW)lParam;
  const int col = info->iSubItem;
  SortOrder order = AscendingOrder;

  if(col == m_sortColumn) {
    switch(m_sortOrder) {
    case AscendingOrder:
      order = DescendingOrder;
      break;
    case DescendingOrder:
      order = AscendingOrder;
      break;
    }
  }

  sortByColumn(col, order);
}

int ListView::translate(const int userIndex) const
{
  if(m_sortColumn < 0 || userIndex < 0)
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
  if(m_sortColumn < 0 || internalIndex < 0)
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

void ListView::restore(const string &data, const int userVersion)
{
  m_userVersion = userVersion; // for save()
  setExStyle(LVS_EX_HEADERDRAGDROP, true); // enable column reordering

  int col = -2;
  vector<int> order(m_columnSize);
  istringstream stream(data);

  while(true) {
    string line, first, second;
    getline(stream, line, RECORD_END);

    istringstream lineStream(line);
    getline(lineStream, first, FIELD_END);
    getline(lineStream, second, FIELD_END);

    int left, right;

    try {
      left = stoi(first.c_str());
      right = stoi(second.c_str());
    }
    catch(logic_error &) {
      return; // data is invalid! aborting.
    }

    switch(col) {
    case -2: // version
      if(left != userVersion || right != VERSION)
        return;
      break;
    case -1: // sort
      if(left < m_columnSize)
        sortByColumn(left, right == 0 ? AscendingOrder : DescendingOrder);
      break;
    default:
      order[col] = left;
      // raw size should not go through adjustSize (via resizeColumn)
      ListView_SetColumnWidth(handle(), col, right);
      break;
    }

    if(stream.eof() || ++col >= m_columnSize)
      break;
  }

  // finish filling for other columns
  for(col++; col < m_columnSize; col++)
    order[col] = col;

  ListView_SetColumnOrderArray(handle(), m_columnSize, &order[0]);
}

string ListView::save() const
{
  vector<int> order(m_columnSize);
  ListView_GetColumnOrderArray(handle(), m_columnSize, &order[0]);

  ostringstream stream;

  stream
    << m_userVersion << FIELD_END
    << VERSION << RECORD_END;

  stream
    << m_sortColumn << FIELD_END
    << m_sortOrder << RECORD_END;

  int i = 0;
  while(true) {
    stream
      << order[i] << FIELD_END
      << columnSize(i);

    if(++i < m_columnSize)
      stream << RECORD_END;
    else
      break;
  }

  return stream.str();
}
