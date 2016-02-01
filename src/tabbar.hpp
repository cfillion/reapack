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

#ifndef REAPACK_TABBAR_HPP
#define REAPACK_TABBAR_HPP

#include "control.hpp"

#include <vector>

#include "encoding.hpp"

class TabBar : public Control {
public:
  typedef std::vector<HWND> Page;
  struct Tab { auto_string text; Page page; };
  typedef std::vector<Tab> Tabs;

  TabBar(const Tabs &tabs, HWND handle);
  int addTab(const Tab &);
  int currentIndex() const;
  void removeTab(const int);

protected:
  void onNotify(LPNMHDR, LPARAM) override;

private:
  void switchPage();

  int m_size;
  int m_lastPage;
  std::vector<Page> m_pages;
};

#endif
