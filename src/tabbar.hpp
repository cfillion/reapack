/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2018  Christian Fillion
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

#include <boost/signals2.hpp>
#include <vector>

class Dialog;

class TabBar : public Control {
public:
  typedef boost::signals2::signal<void (int index)> TabSignal;

  typedef std::vector<HWND> Page;
  struct Tab { const char *text; Page page; };
  typedef std::vector<Tab> Tabs;

  TabBar(HWND handle, Dialog *parent, const Tabs &tabs = {});
  int addTab(const Tab &);
  int currentIndex() const;
  void setCurrentIndex(int);
  void setFocus();
  int count() const;
  void clear();
  void onTabChange(const TabSignal::slot_type &slot) { m_onTabChange.connect(slot); }

protected:
  void onNotify(LPNMHDR, LPARAM) override;

private:
  void switchPage();

  Dialog *m_parent;
  int m_lastPage;
  std::vector<Page> m_pages;
  TabSignal m_onTabChange;
};

#endif
