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

#ifndef REAPACK_BROWSER_HPP
#define REAPACK_BROWSER_HPP

#include "dialog.hpp"

#include "listview.hpp"
#include "registry.hpp"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

class Index;
class ListView;
class ReaPack;
class Version;

typedef std::shared_ptr<const Index> IndexPtr;

class Browser : public Dialog {
public:
  Browser(const std::vector<IndexPtr> &, ReaPack *);
  void reload();

protected:
  void onInit() override;
  void onCommand(int) override;
  void onContextMenu(HWND, int x, int y) override;
  void onTimer(int) override;

private:
  enum Flag {
    UninstalledFlag = 1<<1,
    InstalledFlag   = 1<<2,
    OutOfDateFlag   = 1<<3,
    ObsoleteFlag    = 1<<4,
  };

  struct Entry {
    int flags;
    Registry::Entry regEntry;
    const Package *package;
    const Version *latest;
    const Version *current;

    bool test(Flag f) const { return flags & f; }
  };

  enum Column {
    StateColumn,
    NameColumn,
    CategoryColumn,
    VersionColumn,
    AuthorColumn,
    TypeColumn,
    RemoteColumn,
  };

  static Entry makeEntry(const Package *, const Registry::Entry &);

  bool match(const Entry &) const;
  void checkFilter();
  void fillList();
  std::string getValue(Column, const Entry &entry) const;
  ListView::Row makeRow(const Entry &) const;
  const Entry *getEntry(int) const;
  void selectionMenu();
  bool hasAction(const Entry *e) const { return m_actions.count(e) > 0; }
  bool isTarget(const Entry *, const Version *) const;
  void setAction(const int index, const Version *);
  void selectionDo(const std::function<void (int)> &);
  void apply();

  void installLatest(int index);
  void reinstall(int index);
  void installVersion(int index, size_t verIndex);
  void uninstall(int index);
  void resetAction(int index);
  void history(int index) const;
  void about(int index) const;

  std::vector<IndexPtr> m_indexes;
  ReaPack *m_reapack;
  bool m_checkFilter;
  int m_currentIndex;

  int m_filterTimer;
  std::string m_filter;
  std::vector<Entry> m_entries;
  std::vector<size_t> m_visibleEntries;
  std::map<const Entry *, const Version *> m_actions;

  HWND m_filterHandle;
  HWND m_display;
  std::map<int, HWND> m_types;
  ListView *m_list;
  HWND m_action;
};

#endif
