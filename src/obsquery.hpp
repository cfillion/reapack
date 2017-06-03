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

#ifndef REAPACK_OBSOLETE_QUERY_HPP
#define REAPACK_OBSOLETE_QUERY_HPP

#include "dialog.hpp"

#include "registry.hpp"

#include <vector>

class Config;
class ListView;

class ObsoleteQuery : public Dialog {
public:
  ObsoleteQuery(std::vector<Registry::Entry> *, bool *enable);

protected:
  void onInit() override;
  void onCommand(int, int) override;

private:
  void prepare();

  std::vector<Registry::Entry> *m_entries;
  bool *m_enable;

  HWND m_enableCtrl;
  HWND m_okBtn;
  ListView *m_list;
};

#endif
