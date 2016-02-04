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

#include <vector>

#include "report.hpp"

class ListView;
class Package;
class Remote;
class RemoteIndex;
class ReportBase;
class RichEdit;
class TabBar;
struct Link;

class About : public Dialog {
public:
  enum { EnableResult = 100 };
  About(const Remote *, const RemoteIndex *);

protected:
  void onInit() override;
  void onCommand(int) override;
  void onContextMenu(HWND, int x, int y) override;

private:
  void populate();
  void updatePackages();
  void updateInstalledFiles();
  void selectLink(int control, const std::vector<const Link *> &);
  void openLink(const Link *);
  void packageHistory();

  const Remote *m_remote;
  const RemoteIndex *m_index;
  int m_currentCat;

  TabBar *m_tabs;
  RichEdit *m_about;
  ListView *m_cats;
  ListView *m_packages;
  HWND m_installedFiles;

  std::vector<const Link *> m_websiteLinks;
  std::vector<const Link *> m_donationLinks;
  const std::vector<const Package *> *m_packagesData;
};

class History : public ReportDialog {
public:
  History(const Package *);

protected:
  void fillReport() override;

private:
  const Package *m_package;
};

#endif
