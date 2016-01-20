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
#include "remote.hpp"
#include "resource.hpp"

static const int ACTION_ENABLE = 300;
static const int ACTION_DISABLE = 301;
static const int ACTION_UNINSTALL = 302;

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
  // It would be better to not hard-code the column sizes like that...
#ifndef _WIN32
  const int URL_WIDTH = 360;
#else
  const int URL_WIDTH = 300;
#endif

  m_list = new ListView({
    {AUTO_STR("Name"), 110},
    {AUTO_STR("URL"), URL_WIDTH},
    {AUTO_STR("State"), 60},
  }, getItem(IDC_LIST));
}

void Manager::onCommand(WPARAM wParam, LPARAM)
{
  switch(LOWORD(wParam)) {
  case IDC_IMPORT:
    m_reapack->importRemote();
    break;
  case ACTION_ENABLE:
    setRemoteEnabled(true);
    break;
  case ACTION_DISABLE:
    setRemoteEnabled(false);
    break;
  case ACTION_UNINSTALL:
    uninstall();
    break;
  case IDOK:
    apply();
  case IDCANCEL:
    reset();
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

  const UINT enableAction =
    menu.addAction(AUTO_STR("Enable"), ACTION_ENABLE);
  const UINT disableAction =
    menu.addAction(AUTO_STR("Disable"), ACTION_DISABLE);

  menu.addSeparator();

  const UINT uninstallAction =
    menu.addAction(AUTO_STR("Uninstall"), ACTION_UNINSTALL);

  menu.disable();

  const Remote &remote = currentRemote();

  if(!remote.isNull()) {
    if(isRemoteEnabled(remote))
      menu.enable(disableAction);
    else
      menu.enable(enableAction);

    if(!remote.isProtected())
      menu.enable(uninstallAction);
  }

  menu.show(x, y, handle());
}

void Manager::refresh()
{
  m_list->clear();

  for(const Remote &remote : *m_reapack->config()->remotes()) {
    if(!m_uninstall.count(remote))
      m_list->addRow(makeRow(remote));
  }
}

void Manager::setRemoteEnabled(const bool enabled)
{
  const Remote &remote = currentRemote();

  if(remote.isNull())
    return;

  auto it = m_enableOverrides.find(remote.name());

  if(it == m_enableOverrides.end())
    m_enableOverrides.insert({remote.name(), enabled});
  else if(remote.isEnabled() == enabled)
    m_enableOverrides.erase(it);
  else
    it->second = enabled;

  m_list->replaceRow(m_list->currentIndex(), makeRow(remote));
}

bool Manager::isRemoteEnabled(const Remote &remote) const
{
  const auto it = m_enableOverrides.find(remote.name());

  if(it == m_enableOverrides.end())
    return remote.isEnabled();
  else
    return it->second;
}

void Manager::uninstall()
{
  const Remote &remote = currentRemote();
  m_uninstall.insert(remote);

  const auto it = m_enableOverrides.find(remote.name());

  if(it != m_enableOverrides.end())
    m_enableOverrides.erase(it);

  m_list->removeRow(m_list->currentIndex());
}

void Manager::apply()
{
  RemoteList *list = m_reapack->config()->remotes();

  for(const auto &pair : m_enableOverrides) {
    const string &name = pair.first;
    const bool enable = pair.second;

    Remote remote = list->get(name);
    remote.setEnabled(enable);

    list->add(remote);

    if(enable)
      m_reapack->synchronize(remote);
  }

  for(const Remote &remote : m_uninstall) {
    m_reapack->uninstall(remote);
    list->remove(remote);
  }

  m_reapack->config()->write();
}

void Manager::reset()
{
  m_enableOverrides.clear();
  m_uninstall.clear();
}

ListView::Row Manager::makeRow(const Remote &remote) const
{
  const auto_string &name = make_autostring(remote.name());
  const auto_string &url = make_autostring(remote.url());

  return {name, url, isRemoteEnabled(remote) ?
    AUTO_STR("Enabled") : AUTO_STR("Disabled")};
}

Remote Manager::currentRemote() const
{
  const int index = m_list->currentIndex();

  if(index < 0)
    return {};

  const ListView::Row &row = m_list->getRow(index);
  const string &remoteName = from_autostring(row[0]);

  return m_reapack->config()->remotes()->get(remoteName);
}
