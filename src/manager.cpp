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

#include "manager.hpp"

#include "config.hpp"
#include "encoding.hpp"
#include "menu.hpp"
#include "reapack.hpp"
#include "resource.hpp"

using namespace std;

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
  const int cmdId = LOWORD(wParam);

  switch(LOWORD(wParam)) {
  case IDC_IMPORT:
    m_reapack->importRemote();
    break;
  case Enable:
  case Disable:
  case Uninstall:
    markFor(static_cast<Action>(cmdId));
    break;
  case IDOK:
    apply();
  case IDCANCEL:
    hide();
    break;
  }
}

void Manager::onNotify(LPNMHDR info, LPARAM lParam)
{
  switch(info->idFrom) {
  case IDC_LIST:
    m_list->onNotify(info, lParam);
    break;
  }
}

void Manager::onContextMenu(HWND target, const int x, const int y)
{
  if(target != m_list->handle())
    return;

  Menu menu;

  menu.addAction(AUTO_STR("Enable"), Enable);
  const UINT disableAction =
    menu.addAction(AUTO_STR("Disable"), Disable);

  menu.addSeparator();

  const UINT uninstallAction =
    menu.addAction(AUTO_STR("Uninstall"), Uninstall);

  menu.disable();

  const int index = m_list->currentIndex();

  if(index > -1) {
    menu.enable(disableAction);
    menu.enable(uninstallAction);
  }

  menu.show(x, y, handle());
}

void Manager::refresh()
{
  m_list->clear();

  const RemoteMap remotes = m_reapack->config()->remotes();
  for(auto it = remotes.begin(); it != remotes.end(); it++)
    m_list->addRow(makeRow(*it));
}

void Manager::markFor(const Action action)
{
  const int index = m_list->currentIndex();

  if(index < 0)
    return;

  const string remoteName = from_autostring(m_list->getRow(index)[0]);

  const RemoteMap &remotes = m_reapack->config()->remotes();
  const auto it = remotes.find(remoteName);

  if(it == remotes.end())
    return;

  const Remote remote = {"aaaaaaa", it->second};

  MessageBox(handle(), to_autostring(action).c_str(),
    m_list->getRow(index)[0].c_str(), MB_OK);

  m_list->replaceRow(index, makeRow(remote));
}

void Manager::apply()
{
}

ListView::Row Manager::makeRow(const Remote &remote)
{
  const auto_string name = make_autostring(remote.first);
  const auto_string url = make_autostring(remote.second);

  return {name, url, AUTO_STR("Enabled")};
}
