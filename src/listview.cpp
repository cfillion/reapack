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

#ifdef _WIN32
#include <commctrl.h>
#endif

using namespace std;

ListView::ListView(const Columns &columns, HWND handle)
  : Control(handle), m_columnSize(0), m_rowSize(0)
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
  item.cx = col.width;

  item.mask |= LVCF_TEXT;
  item.pszText = const_cast<auto_char *>(col.text.c_str());

  ListView_InsertColumn(handle(), m_columnSize++, &item);
}

int ListView::addRow(const Row &content)
{
  LVITEM item{};
  item.iItem = m_rowSize++;

  ListView_InsertItem(handle(), &item);

  replaceRow(item.iItem, content);

  return item.iItem;
}

void ListView::replaceRow(const int index, const Row &content)
{
  LVITEM item{};
  item.iItem = index;

  const int cols = min(m_columnSize, (int)content.size());

  for(int i = 0; i < cols; i++) {
    auto_char *text = const_cast<auto_char *>(content[i].c_str());
    ListView_SetItemText(handle(), item.iItem, i, text);
  }
}

void ListView::removeRow(const int index)
{
  ListView_DeleteItem(handle(), index);
}

void ListView::resizeColumn(const int index, const int width)
{
  ListView_SetColumnWidth(handle(), index, width);
}

ListView::Row ListView::getRow(const int rowIndex) const
{
  Row row(m_columnSize);

  for(int i = 0; i < m_columnSize; i++) {
    auto_char buf[4096];
    ListView_GetItemText(handle(), rowIndex, i, buf, sizeof(buf));
    row[i] = buf;
  }

  return row;
}

void ListView::clear()
{
  ListView_DeleteAllItems(handle());

  m_rowSize = 0;
}

bool ListView::hasSelection() const
{
  return currentIndex() > -1;
}

int ListView::currentIndex() const
{
  return ListView_GetNextItem(handle(), -1, LVNI_SELECTED);
}

void ListView::onNotify(LPNMHDR info, LPARAM)
{
  switch(info->code) {
  case LVN_ITEMCHANGED:
    m_onSelect();
    break;
  case NM_DBLCLK:
    m_onDoubleClick();
    break;
  };
}
