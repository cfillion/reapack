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

#include <map>
#include <memory>
#include <vector>

class Index;
class ListView;
class Menu;
class Metadata;
class Package;
class RichEdit;
class Source;
class TabBar;
struct Link;

typedef std::shared_ptr<const Index> IndexPtr;

class AboutDialog : public Dialog {
public:
  AboutDialog(const Metadata *);

protected:
  void onInit() override;
  void onCommand(int, int) override;

  virtual const std::string &what() const = 0;
  virtual ListView *createMenu() = 0;
  virtual ListView *createList() = 0;

  virtual void populate() = 0;
  virtual void updateList(int) = 0;

  TabBar *tabs() const { return m_tabs; }
  ListView *menu() const { return m_menu; }
  ListView *list() const { return m_list; }
  HWND report() const { return m_report; }

private:
  void populateLinks();
  void selectLink(int control);
  void openLink(const Link *);
  void callUpdateList();

  const Metadata *m_metadata;
  int m_currentIndex;
  TabBar *m_tabs;
  ListView *m_menu;
  ListView *m_list;
  HWND m_report;

  std::map<int, std::vector<const Link *> > m_links;
};

class AboutRemote : public AboutDialog {
public:
  enum { InstallResult = 100 };

  AboutRemote(const IndexPtr &);

protected:
  const std::string &what() const override;
  ListView *createMenu() override;
  ListView *createList() override;

  void onCommand(int, int) override;
  void populate() override;
  void updateList(int) override;

private:
  bool fillContextMenu(Menu &, int) const;
  void updateInstalledFiles();
  void aboutPackage();

  IndexPtr m_index;
  const std::vector<const Package *> *m_packagesData;
};

class AboutPackage : public AboutDialog {
public:
  AboutPackage(const Package *);

protected:
  const std::string &what() const override;
  ListView *createMenu() override;
  ListView *createList() override;

  void onCommand(int, int) override;
  void populate() override;
  void updateList(int) override;

private:
  bool fillContextMenu(Menu &, int) const;
  void copySourceUrl();

  const Package *m_package;
  std::vector<const Source *> m_sources;
};

#endif
