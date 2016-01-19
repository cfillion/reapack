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

#ifndef REAPACK_MANAGER_HPP
#define REAPACK_MANAGER_HPP

#include "dialog.hpp"

#include "listview.hpp"

#include <map>
#include <set>

class ReaPack;
class Remote;

class Manager : public Dialog {
public:
  Manager(ReaPack *);
  ~Manager();

  void refresh();

protected:
  void onInit() override;
  void onCommand(WPARAM, LPARAM) override;
  void onNotify(LPNMHDR, LPARAM) override;
  void onContextMenu(HWND, int x, int y) override;

private:
  ListView::Row makeRow(const Remote &remote) const;

  Remote currentRemote() const;
  void setRemoteEnabled(const bool);
  bool isRemoteEnabled(const Remote &remote) const;
  void uninstall();

  void apply();
  void reset();

  ReaPack *m_reapack;
  ListView *m_list;

  std::map<std::string, bool> m_enableOverrides;
  std::set<Remote> m_uninstall;
};

#endif
