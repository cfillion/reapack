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
  struct Entry { const Version *version; Registry::Entry regEntry; };

  enum Column {
    StateColumn,
    NameColumn,
    CategoryColumn,
    VersionColumn,
    AuthorColumn,
    TypeColumn,
  };

  bool match(const Entry &) const;
  void checkFilter();
  void fillList();
  std::string getValue(Column, const Entry &entry) const;
  ListView::Row makeRow(const Entry &) const;

  std::vector<IndexPtr> m_indexes;
  ReaPack *m_reapack;
  bool m_checkFilter;
  int m_filterTimer;
  std::string m_filter;
  std::vector<Entry> m_entries;

  HWND m_filterHandle;
  HWND m_display;
  std::map<int, HWND> m_types;
  ListView *m_list;
};

#endif
