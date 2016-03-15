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

#ifndef REAPACK_CLEANUP_HPP
#define REAPACK_CLEANUP_HPP

#include "dialog.hpp"

#include "registry.hpp"

#include <memory>
#include <vector>

class Index;
class ListView;
class ReaPack;

typedef std::shared_ptr<const Index> IndexPtr;

class Cleanup : public Dialog {
public:
  Cleanup(const std::vector<IndexPtr> &, ReaPack *);

protected:
  void onInit() override;
  void onCommand(int) override;
  void onContextMenu(HWND, int x, int y) override;
  void onTimer(int) override;

private:
  void populate();
  void onSelectionChanged();
  bool confirm() const;
  void apply();

  std::vector<IndexPtr> m_indexes;
  ReaPack *m_reapack;

  HWND m_ok;
  HWND m_stateLabel;
  ListView *m_list;

  std::vector<Registry::Entry> m_entries;
};

#endif
