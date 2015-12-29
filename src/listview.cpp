/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015  Christian Fillion
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

using namespace std;

ListView::ListView(const Columns &columns, HWND handle)
  : m_handle(handle), m_columnSize(0), m_rowSize(0)
{
  for(const Column &col : columns)
    addColumn(col.first, col.second);
}

void ListView::addColumn(const auto_char *text, const int width)
{
  LVCOLUMN col = {0};

  col.mask |= LVCF_WIDTH;
  col.cx = width;

  col.mask |= LVCF_TEXT;
  col.pszText = const_cast<auto_char *>(text);

  ListView_InsertColumn(m_handle, m_columnSize++, &col);
}

void ListView::addRow(const Row &content)
{
  LVITEM item = {0};
  item.iItem = m_rowSize++;

  ListView_InsertItem(m_handle, &item);

  const size_t contentSize = min(m_columnSize, content.size());

  for(size_t i = 0; i < contentSize; i++)
    ListView_SetItemText(m_handle, item.iItem, i, content[i]);
}

void ListView::clear()
{
  for(size_t i = 0; i < m_rowSize; i++)
    ListView_DeleteItem(m_handle, 0);

  m_rowSize = 0;
}
