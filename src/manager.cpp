/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015  Christian Fillion
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

#include "manager.hpp"

#include "config.hpp"
#include "encoding.hpp"
#include "reapack.hpp"
#include "resource.hpp"

const auto_char *NAME = AUTO_STR("Name");
const auto_char *URL = AUTO_STR("URL");

Manager::Manager(ReaPack *reapack)
  : Dialog(IDD_CONFIG_DIALOG), m_reapack(reapack), m_list(0)
{
}

Manager::~Manager()
{
  delete m_list;
}

void Manager::onInit()
{
  m_list = new ListView({
    {AUTO_STR("Name"), 100},
    {AUTO_STR("URL"), 360},
    {AUTO_STR("State"), 60},
  }, getItem(IDC_LIST));
}

void Manager::onCommand(WPARAM wParam, LPARAM)
{
  switch(LOWORD(wParam)) {
  case IDC_IMPORT:
    m_reapack->importRemote();
    break;
  case IDC_UNINSTALL:
    MessageBox(handle(), AUTO_STR("Not Implemented"),
      AUTO_STR("Uninstall"), MB_OK);
    break;
  case IDOK:
    apply();
  case IDCANCEL:
    hide();
    break;
  }
}

void Manager::refresh()
{
  m_list->clear();

  const RemoteMap remotes = m_reapack->config()->remotes();
  for(auto it = remotes.begin(); it != remotes.end(); it++) {
    const auto_string &name = make_autostring(it->first);
    const auto_string &url = make_autostring(it->second);

    m_list->addRow({name.c_str(), url.c_str(), AUTO_STR("Enabled")});
  }
}

void Manager::apply()
{
}
