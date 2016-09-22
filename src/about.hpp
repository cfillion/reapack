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

class AboutDelegate;
class Index;
class ListView;
class Menu;
class Metadata;
class Package;
class ReaPack;
class RichEdit;
class Source;
class TabBar;
struct Link;

typedef std::shared_ptr<const Index> IndexPtr;

class About : public Dialog {
public:
  typedef std::shared_ptr<AboutDelegate> DelegatePtr;

  About();
  void setDelegate(const DelegatePtr &, bool focus = true);
  template<typename T>
  bool testDelegate() { return dynamic_cast<T *>(m_delegate.get()) != nullptr; }

  void setTitle(const std::string &);
  void setMetadata(const Metadata *, bool substitution = false);
  void setAction(const std::string &);

  TabBar *tabs() const { return m_tabs; }
  RichEdit *desc() const { return m_desc; }
  ListView *menu() const { return m_menu; }
  ListView *list() const { return m_list; }

protected:
  void onInit() override;
  void onCommand(int, int) override;

private:
  void selectLink(int control);
  void openLink(const Link *);
  void updateList();

  int m_currentIndex;
  std::map<int, std::vector<const Link *> > m_links;

  TabBar *m_tabs;
  RichEdit *m_desc;
  ListView *m_menu;
  ListView *m_list;

  DelegatePtr m_delegate;
};

class AboutDelegate {
protected:
  friend About;

  virtual void init(About *) = 0;
  virtual void updateList(int) {}
  virtual bool fillContextMenu(Menu &, int) const { return false; }
  virtual void itemActivated() {};
  virtual void onCommand(int) {};
};

class AboutIndexDelegate : public AboutDelegate {
public:
  AboutIndexDelegate(const IndexPtr &, ReaPack *);

protected:
  void init(About *) override;
  void updateList(int) override;
  bool fillContextMenu(Menu &, int) const override;
  void itemActivated() override { aboutPackage(); }
  void onCommand(int) override;

private:
  void initInstalledFiles();
  const Package *currentPackage() const;
  void findInBrowser();
  void aboutPackage();
  void install();

  IndexPtr m_index;
  const std::vector<const Package *> *m_packagesData;

  ReaPack *m_reapack;
  About *m_dialog;
};

class AboutPackageDelegate : public AboutDelegate {
public:
  AboutPackageDelegate(const Package *, ReaPack *);

protected:
  void init(About *) override;
  void updateList(int) override;
  bool fillContextMenu(Menu &, int) const override;
  void onCommand(int) override;

private:
  void copySourceUrl();

  const Package *m_package;
  ReaPack *m_reapack;
  IndexPtr m_index; // keeps the package loaded in memory
  const std::vector<const Source *> *m_sources;

  About *m_dialog;
};

#endif
