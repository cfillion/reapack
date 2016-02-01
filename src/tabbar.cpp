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

#include "tabbar.hpp"

#ifdef _WIN32
#include <commctrl.h>
#endif

TabBar::TabBar(const Tabs &tabs, HWND handle)
  : Control(handle), m_size(0), m_lastPage(0)
{
  for(const Tab &tab : tabs)
    addTab(tab);
}

int TabBar::addTab(const Tab &tab)
{
  int index = m_size++;

  for(HWND control : tab.page)
    ShowWindow(control, SW_HIDE);

  m_pages.push_back(tab.page);

  TCITEM item{};
  item.mask |= TCIF_TEXT;
  item.pszText = const_cast<auto_char *>(tab.text.c_str());

  TabCtrl_InsertItem(handle(), index, &item);

  if(index == 0)
    switchPage();

  return index;
}

int TabBar::currentIndex() const
{
  return TabCtrl_GetCurSel(handle());
}

void TabBar::removeTab(const int index)
{
  if(TabCtrl_DeleteItem(handle(), index)) {
    m_pages.erase(m_pages.begin() + index);
    switchPage();
  }
}

void TabBar::onNotify(LPNMHDR info, LPARAM)
{
  switch(info->code) {
  case TCN_SELCHANGE:
    switchPage();
    break;
  };
}

void TabBar::switchPage()
{
  if(m_lastPage > -1) {
    for(HWND control : m_pages[m_lastPage])
      ShowWindow(control, SW_HIDE);
  }

  const int index = currentIndex();

  if(index < 0 || (size_t)index >= m_pages.size()) {
    m_lastPage = -1;
    return;
  }

  const Page &page = m_pages[index];
  m_lastPage = index;

  if(page.empty())
    return;

  for(HWND control : page)
    ShowWindow(control, SW_SHOW);

  SetFocus(page.front());
}
