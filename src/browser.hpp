/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2017  Christian Fillion
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

#ifndef REAPACK_BROWSER_HPP
#define REAPACK_BROWSER_HPP

#include "dialog.hpp"

#include "filter.hpp"
#include "listview.hpp"
#include "registry.hpp"

#include <boost/optional.hpp>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

class Index;
class ListView;
class Menu;
class ReaPack;
class Remote;
class Version;

typedef std::shared_ptr<const Index> IndexPtr;

class Browser : public Dialog {
public:
  Browser(ReaPack *);
  void refresh(bool stale = false);
  void setFilter(const std::string &);

protected:
  void onInit() override;
  void onCommand(int, int) override;
  void onTimer(int) override;
  bool onKeyDown(int, int) override;
  void onClose() override;

private:
  enum Flag {
    UninstalledFlag = 1<<0,
    InstalledFlag   = 1<<1,
    OutOfDateFlag   = 1<<2,
    ObsoleteFlag    = 1<<3,
  };

  struct Entry {
    typedef std::tuple<std::string, std::string, std::string> Hash;

    int flags;
    Registry::Entry regEntry;
    IndexPtr index;
    const Package *package;
    const Version *latest;
    const Version *current;

    boost::optional<const Version *> target;
    boost::optional<bool> pin;

    Hash hash() const;
    bool test(Flag f) const { return (flags & f) != 0; }
    bool canPin() const { return target ? *target != nullptr : test(InstalledFlag); }
    bool operator==(const Entry &o) const { return hash() == o.hash(); }
  };

  enum Column {
    StateColumn,
    NameColumn,
    CategoryColumn,
    VersionColumn,
    AuthorColumn,
    TypeColumn,
    RemoteColumn,
    TimeColumn,
  };

  enum View {
    AllView,
    QueuedView,
    InstalledView,
    OutOfDateView,
    ObsoleteView,
    UninstalledView,
  };

  enum LoadState {
    Init,
    Loaded,
    Loading,
    Done,
  };

  Entry makeEntry(const Package *, const Registry::Entry &, const IndexPtr &) const;

  void onSelection();
  bool fillContextMenu(Menu &, int index);
  void populate(const std::vector<IndexPtr> &);
  void transferActions();
  bool match(const Entry &) const;
  void updateFilter();
  void updateAbout();
  void fillList();
  std::string getValue(Column, const Entry &entry) const;
  ListView::Row makeRow(const Entry &) const;
  Entry *getEntry(int);
  Remote getRemote(const Entry &) const;
  void updateDisplayLabel();
  void displayButton();
  void actionsButton();
  void fillMenu(Menu &);
  bool isFiltered(Package::Type) const;
  void toggleDescs();
  bool hasAction(const Entry *) const;
  void setTarget(const int index, const Version *, bool toggle = true);
  void resetTarget(int index);
  void resetActions(int index);
  void updateAction(const int index);
  void selectionDo(const std::function<void (int)> &);
  View currentView() const;
  bool confirm() const;
  bool apply();

  void installLatest(int index, bool toggle = true);
  void installLatestAll();
  void reinstall(int index, bool toggle = true);
  void installVersion(int index, size_t verIndex);
  void uninstall(int index, bool toggle = true);
  void togglePin(int index);
  void aboutRemote(int index, bool focus = true);
  void aboutPackage(int index, bool focus = true);

  ReaPack *m_reapack;
  LoadState m_loadState;
  int m_currentIndex;

  Filter m_filter;
  boost::optional<Package::Type> m_typeFilter;
  std::vector<Entry> m_entries;
  std::vector<size_t> m_visibleEntries;
  std::list<Entry *> m_actions;

  HWND m_filterHandle;
  HWND m_view;
  HWND m_displayBtn;
  HWND m_actionsBtn;
  ListView *m_list;
  HWND m_applyBtn;
  Serializer m_serializer;
};

#endif
