/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2020  Christian Fillion
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

#include "iconlist.hpp"
#include "menu.hpp"
#include "time.hpp"
#include "version.hpp"
#include "win32.hpp"

#include <boost/algorithm/string/case_conv.hpp>
#include <cassert>
#include <reaper_plugin_secrets.h>

static int adjustWidth(const int points)
{
#ifdef _WIN32
  if(points < 1)
    return points;
  else // magic number to make pretty sizes...
    return static_cast<int>(std::ceil(points * 0.863));
#else
  return points;
#endif
}

ListView::ListView(HWND handle, const Columns &columns)
  : Control(handle), m_dirty(0), m_customizable(false), m_sort(), m_defaultSort()
{
  for(const Column &col : columns)
    addColumn(col);

  int style = LVS_EX_FULLROWSELECT;

#ifdef LVS_EX_LABELTIP
  // unsupported by SWELL, but always enabled on macOS anyway
  style |= LVS_EX_LABELTIP;
#endif

#ifdef LVS_EX_DOUBLEBUFFER
  style |= LVS_EX_DOUBLEBUFFER;
#endif

  setExStyle(style);
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

ListView::Row *ListView::createRow(void *data)
{
  const int index = rowCount();
  insertItem(index, index);

  return m_rows.emplace_back(std::make_unique<Row>(data, this)).get();
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

void ListView::enableIcons()
{
  static IconList list({IconList::UncheckedIcon, IconList::CheckedIcon});

  // NOTE: the list must have the LVS_SHAREIMAGELISTS style to prevent
  // it from taking ownership of the image list
  ListView_SetImageList(handle(), list.handle(), LVSIL_SMALL);
}

void ListView::setRowIcon(const int row, const int image)
{
  LVITEM item{};
  item.iItem = translate(row);
  item.iImage = image;
  item.mask |= LVIF_IMAGE;

  ListView_SetItem(handle(), &item);
}

void ListView::removeRow(const int userIndex)
{
  // translate to view index before fixing lParams
  const int viewIndex = translate(userIndex);

  // shift lParam and userIndex of subsequent rows to reflect the new indexes
  const int size = rowCount();
  for(int i = userIndex + 1; i < size; i++) {
    m_rows[i]->userIndex = i - 1;

    LVITEM item{};
    item.iItem = translate(i);
    item.mask |= LVIF_PARAM;
    item.lParam = m_rows[i]->userIndex;
    ListView_SetItem(handle(), &item);
  }

  ListView_DeleteItem(handle(), viewIndex);
  m_rows.erase(m_rows.begin() + userIndex);

  reindexVisible(); // do it now so further removeRow will work as expected
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
    const int indexDiff = static_cast<int>(aRow - bRow);

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

  ListView_SortItems(handle(), compare, reinterpret_cast<LPARAM>(this));

  m_dirty = (m_dirty | NeedReindexFlag) & ~NeedSortFlag;
}

void ListView::sortByColumn(const int index, const SortOrder order, const bool user)
{
  if(m_sort)
    setSortArrow(false);

  const Sort settings{index, order};

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
  std::vector<int> hide;

  for(int ri = 0; ri < rowCount(); ++ri) {
    const auto &row = m_rows[ri];

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
  for(int i = 0; i < static_cast<int>(hide.size()); ++i) {
    ListView_DeleteItem(handle(), hide[i] - i);
    m_dirty |= NeedReindexFlag;
  }

  m_dirty &= ~NeedFilterFlag;
}

void ListView::setFilter(const std::string &newFilter)
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
  m_sort = std::nullopt;
  m_defaultSort = std::nullopt;
}

void ListView::setSelected(const int index, const bool select)
{
  ListView_SetItemState(handle(), translate(index),
    select ? LVIS_SELECTED : 0, LVIS_SELECTED);
}

void ListView::selectAll()
{
  InhibitControl inhibit(this);
  select(-1);
}

void ListView::unselectAll()
{
  InhibitControl inhibit(this);
  unselect(-1);
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

std::vector<int> ListView::selection(const bool sort) const
{
  int index = -1;
  std::vector<int> selectedIndexes;

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

bool ListView::headerHitTest(const int x, const int y) const
{
#ifdef _WIN32
  RECT rect;
  GetWindowRect(ListView_GetHeader(handle()), &rect);

  const int headerHeight = rect.bottom - rect.top;
#elif !defined(__APPLE__)
  const int headerHeight = SWELL_GetListViewHeaderHeight(handle());
#endif

  POINT point{x, y};
  ScreenToClient(handle(), &point);

#ifdef __APPLE__
  // This was broken on Linux and used a hard-coded header height on Windows
  // Fixed in REAPER v6.03
  return ListView_HeaderHitTest(handle(), point);
#else
  return point.y <= headerHeight;
#endif
}

int ListView::itemUnder(const int x, const int y, bool *overIcon) const
{
  LVHITTESTINFO test{{x, y}};
  ScreenToClient(handle(), &test.pt);
  ListView_SubItemHitTest(handle(), &test);

  if(overIcon) {
    *overIcon = test.iSubItem == 0 &&
      (test.flags & (LVHT_ONITEMICON | LVHT_ONITEMSTATEICON)) != 0 &&
      (test.flags & LVHT_ONITEMLABEL) == 0;
  }

  return translateBack(test.iItem);
}

int ListView::itemUnderMouse(bool *overIcon) const
{
  POINT point;
  GetCursorPos(&point);
  return itemUnder(point.x, point.y, overIcon);
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
    onItemChanged(lParam);
    break;
  case NM_CLICK:
  case NM_DBLCLK:
    onClick(info->code == NM_DBLCLK);
    break;
  case LVN_COLUMNCLICK:
    onColumnClick(lParam);
    break;
  };
}

bool ListView::onContextMenu(HWND dialog, int x, int y)
{
  SetFocus(handle());

  const bool keyboardTrigger = x < 0;

  if(!keyboardTrigger && headerHitTest(x, y)) {
    if(m_customizable) // show menu only if header is customizable
      headerMenu(x, y);
    return true;
  }

#ifdef ListView_GetItemPosition // unsuported by SWELL
  int index;

  // adjust the context menu's position when using Shift+F10 on Windows
  if(keyboardTrigger) {
    index = currentIndex();

    // find the location of the current item or of the first item
    POINT itemPos{};
    ListView_GetItemPosition(handle(), translate(std::max(0, index)), &itemPos);
    ClientToScreen(handle(), &itemPos);

    RECT controlRect;
    GetWindowRect(handle(), &controlRect);

    x = std::max(controlRect.left, std::min(itemPos.x, controlRect.right));
    y = std::max(controlRect.top, std::min(itemPos.y, controlRect.bottom));
  }
  else
    index = itemUnder(x, y);
#else
  const int index = itemUnder(x, y);
#endif

  Menu menu;

  if(!onFillContextMenu(menu, index).value_or(false))
    return false;

  menu.show(x, y, dialog);

  return true;
}

void ListView::onItemChanged(const LPARAM lParam)
{
#ifdef _WIN32
  const auto info = reinterpret_cast<LPNMLISTVIEW>(lParam);

  if(info->uChanged & LVIF_STATE)
#endif
    onSelect();
}

void ListView::onClick(const bool dbclick)
{
  bool overIcon;

  if(itemUnderMouse(&overIcon) > -1 && currentIndex() > -1) {
    if(dbclick)
      onActivate();
    else if(overIcon)
      onIconClick();
  }
}

void ListView::onColumnClick(const LPARAM lParam)
{
  const auto info = reinterpret_cast<LPNMLISTVIEW>(lParam);
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
    return static_cast<int>(item.lParam);
  else
    return -1;
}

void ListView::headerMenu(const int x, const int y)
{
  enum { ACTION_RESTORE = 800 };

  Menu menu;
  menu.disable(menu.addAction("Visible columns:", 0));

  for(int i = 0; i < columnCount(); i++) {
    const auto id = menu.addAction(m_cols[i].label.c_str(), i | (1 << 8));

    if(columnWidth(i))
      menu.check(id);
  }

  menu.addSeparator();
  menu.addAction("Reset columns", ACTION_RESTORE);

  const int cmd = menu.show(x, y, handle());

  if(cmd == ACTION_RESTORE)
    resetColumns();
  else if(cmd >> 8 == 1) {
    const int col = cmd & 0xff;
    resizeColumn(col, columnWidth(col) ? 0 : m_cols[col].width);
  }
}

void ListView::resetColumns()
{
  std::vector<int> order(columnCount());

  for(int i = 0; i < columnCount(); i++) {
    order[i] = i;

    const Column &col = m_cols[i];
    resizeColumn(i, col.test(CollapseFlag) ? 0 : col.width);
  }

  ListView_SetColumnOrderArray(handle(), columnCount(), order.data());

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
  setExStyle(LVS_EX_HEADERDRAGDROP); // enable column reordering

  if(data.empty())
    return;

  int col = -1;
  std::vector<int> order(columnCount());

  while(col < columnCount() && !data.empty()) {
    const auto &[left, right] = data.front();

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

    data.pop_front(); // deletes rec
    ++col;
  }

  // fill default values for any columns whose state wasn't saved
  // (col can't be -1 at this point, the loop above is always run at least once)
  while(col < columnCount()) {
    order[col] = col;
    ++col;
  }

  ListView_SetColumnOrderArray(handle(), columnCount(), order.data());
}

void ListView::saveState(Serializer::Data &data) const
{
  const Sort &sort = m_sort.value_or(Sort{});
  std::vector<int> order(columnCount());
  ListView_GetColumnOrderArray(handle(), columnCount(), order.data());

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
    std::string l = cl.value, r = cr.value;

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
  : userData(data), viewIndex(list->rowCount()), userIndex(viewIndex),
  m_list(list), m_cells(new Cell[m_list->columnCount()])
{
}

void ListView::Row::setCell(const int i, const std::string &val, void *data)
{
  Cell &cell = m_cells[i];
  cell.value = val;
  cell.userData = data;

  m_list->updateCell(userIndex, i);
}

void ListView::Row::setChecked(bool checked)
{
  m_list->setRowIcon(userIndex, checked);
}

std::vector<std::string> ListView::Row::filterValues() const
{
  std::vector<std::string> values;

  for(int ci = 0; ci < m_list->columnCount(); ++ci) {
    if(m_list->column(ci).test(FilterFlag))
      values.push_back(m_cells[ci].value);
  }

  return values;
}
