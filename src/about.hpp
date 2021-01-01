/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2021  Christian Fillion
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

#include "version.hpp"

#include <map>
#include <memory>
#include <vector>

class AboutDelegate;
class Index;
class ListView;
class Menu;
class Metadata;
class RichEdit;
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
  bool onKeyDown(int, int) override;
  void onClose() override;

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
  Serializer m_serializer;
};

class AboutDelegate {
protected:
  friend About;

  virtual ~AboutDelegate() = default;

  virtual void init(About *) = 0;
  virtual void updateList(int) = 0;
  virtual bool fillContextMenu(Menu &, int) const = 0;
  virtual void itemActivated() = 0;
  virtual void itemCopy() = 0;
  virtual void onCommand(int) = 0;

  virtual const void *data() const = 0;
};

class AboutIndexDelegate : public AboutDelegate {
public:
  AboutIndexDelegate(const IndexPtr &);

protected:
  void init(About *) override;
  void updateList(int) override;
  bool fillContextMenu(Menu &, int) const override;
  void itemActivated() override { aboutPackage(); }
  void itemCopy() override;
  void onCommand(int) override;

  const void *data() const override
  {
    return static_cast<const void *>(m_index.get());
  }

private:
  void initInstalledFiles();
  const Package *currentPackage() const;
  void findInBrowser();
  void aboutPackage();
  void install();

  IndexPtr m_index;

  About *m_dialog;
};

class AboutPackageDelegate : public AboutDelegate {
public:
  AboutPackageDelegate(const Package *, const VersionName &current);

protected:
  void init(About *) override;
  void updateList(int) override;
  bool fillContextMenu(Menu &, int) const override;
  void itemActivated() override { locate(); }
  void itemCopy() override { copySourceUrl(); }
  void onCommand(int) override;

  const void *data() const override
  {
    return static_cast<const void *>(m_package);
  }

private:
  const Source *currentSource() const;
  void copySourceUrl();
  void locate();

  const Package *m_package;
  VersionName m_current;
  IndexPtr m_index; // keeps the package loaded in memory

  About *m_dialog;
};

#endif
