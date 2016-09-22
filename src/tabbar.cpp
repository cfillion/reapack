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

#include "dialog.hpp"

#ifdef _WIN32
#include <commctrl.h>
#endif

TabBar::TabBar(HWND handle, Dialog *parent, const Tabs &tabs)
  : Control(handle), m_parent(parent), m_lastPage(-1)
{
  for(const Tab &tab : tabs)
    addTab(tab);
}

int TabBar::addTab(const Tab &tab)
{
  const int index = count();

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

void TabBar::setCurrentIndex(const int index)
{
  TabCtrl_SetCurSel(handle(), index);
  switchPage();
}

void TabBar::removeTab(const int index)
{
  if(currentIndex() == index)
    setCurrentIndex(index == 0 ? index + 1 : index - 1);

  if(TabCtrl_DeleteItem(handle(), index))
    m_pages.erase(m_pages.begin() + index);
}

void TabBar::setFocus()
{
  const int index = currentIndex();

  if(index > -1 && m_parent->hasFocus())
    SetFocus(m_pages[index].front());
}

int TabBar::count() const
{
  return TabCtrl_GetItemCount(handle());
}

void TabBar::clear()
{
  m_pages.clear();

#ifdef TabCtrl_DeleteAllItems
  TabCtrl_DeleteAllItems(handle());
#else
  for(int i = count(); i > 0; i--)
    TabCtrl_DeleteItem(handle(), i - 1);
#endif
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
  InhibitControl block(m_parent->handle());

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

  setFocus();
}
