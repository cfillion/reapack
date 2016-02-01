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

#ifndef REAPACK_ABOUT_HPP
#define REAPACK_ABOUT_HPP

#include "dialog.hpp"

#include "encoding.hpp"

class ListView;
class RemoteIndex;
class TabBar;

class About : public Dialog {
public:
  About(RemoteIndex *index);

protected:
  void onInit() override;
  void onCommand(int) override;

private:
  void populate();
  void updatePackages(bool);
  void setAboutText(const auto_string &);

  RemoteIndex *m_index;

  TabBar *m_tabs;
  HWND m_about;
  ListView *m_cats;
  ListView *m_packages;
};

#endif
