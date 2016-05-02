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

#include "about.hpp"
#include "config.hpp"
#include "encoding.hpp"
#include "errors.hpp"
#include "index.hpp"
#include "menu.hpp"
#include "reapack.hpp"
#include "remote.hpp"
#include "resource.hpp"
#include "transaction.hpp"

using namespace std;

enum { ACTION_ENABLE = 80, ACTION_DISABLE, ACTION_UNINSTALL, ACTION_ABOUT,
       ACTION_AUTOINSTALL, ACTION_BLEEDINGEDGE, ACTION_SELECT, ACTION_UNSELECT };

Manager::Manager(ReaPack *reapack)
  : Dialog(IDD_CONFIG_DIALOG),
    m_reapack(reapack), m_config(reapack->config()), m_list(0)
{
}

void Manager::onInit()
{
  m_apply = getControl(IDAPPLY);
  disable(m_apply);

  m_list = createControl<ListView>(IDC_LIST, ListView::Columns{
    {AUTO_STR("Name"), 110},
    {AUTO_STR("URL"), 350},
    {AUTO_STR("State"), 60},
  });

  m_list->onActivate([=] { about(m_list->currentIndex()); });

  refresh();

#ifdef LVSCW_AUTOSIZE_USEHEADER
  m_list->resizeColumn(m_list->columnCount() - 1, LVSCW_AUTOSIZE_USEHEADER);
#endif
}

void Manager::onCommand(const int id, int)
{
  switch(id) {
  case IDC_IMPORT:
    m_reapack->importRemote();
    break;
  case IDC_OPTIONS:
    options();
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
  case ACTION_AUTOINSTALL:
    m_autoInstall = !m_autoInstall.value_or(m_config->install()->autoInstall);
    enable(m_apply);
    break;
  case ACTION_BLEEDINGEDGE:
    m_bleedingEdge = !m_bleedingEdge.value_or(m_config->install()->bleedingEdge);
    enable(m_apply);
    break;
  case ACTION_SELECT:
    m_list->selectAll();
    SetFocus(m_list->handle());
    break;
  case ACTION_UNSELECT:
    m_list->unselectAll();
    SetFocus(m_list->handle());
    break;
  case IDOK:
  case IDAPPLY:
    if(confirm()) {
      if(!apply() || id == IDAPPLY)
        break;

      // IDOK -> continue to next case (IDCANCEL)
    }
    else {
      m_uninstall.clear();
      refresh();
      break;
    }
  case IDCANCEL:
    close();
    break;
  default:
    if(id >> 8 == ACTION_ABOUT)
      about(id & 0xff);
    break;
  }
}

void Manager::onContextMenu(HWND target, const int x, const int y)
{
  if(target != m_list->handle())
    return;

  const int index = m_list->itemUnderMouse();
  const Remote &remote = getRemote(index);


  Menu menu;

  if(remote.isNull()) {
    menu.addAction(AUTO_STR("&Select all"), ACTION_SELECT);
    menu.addAction(AUTO_STR("&Unselect all"), ACTION_UNSELECT);
    menu.show(x, y, handle());
    return;
  }

  const UINT enableAction =
    menu.addAction(AUTO_STR("&Enable"), ACTION_ENABLE);
  const UINT disableAction =
    menu.addAction(AUTO_STR("&Disable"), ACTION_DISABLE);

  menu.addSeparator();

  const UINT uninstallAction =
    menu.addAction(AUTO_STR("&Uninstall"), ACTION_UNINSTALL);

  menu.addSeparator();

  auto_char aboutLabel[32] = {};
  const auto_string &name = make_autostring(remote.name());
  auto_snprintf(aboutLabel, auto_size(aboutLabel),
    AUTO_STR("&About %s..."), name.c_str());
  menu.addAction(aboutLabel, index | (ACTION_ABOUT << 8));

  menu.disable(enableAction);
  menu.disable(disableAction);
  menu.disable(uninstallAction);

  if(isRemoteEnabled(remote))
    menu.enable(disableAction);
  else
    menu.enable(enableAction);

  if(!remote.isProtected())
    menu.enable(uninstallAction);

  menu.show(x, y, handle());
}

void Manager::refresh()
{
  InhibitControl lock(m_list);

  const vector<int> selection = m_list->selection();
  vector<string> selected(selection.size());
  for(size_t i = 0; i < selection.size(); i++)
    selected[i] = m_list->row(selection[i])[0];

  m_list->clear();

  for(const Remote &remote : *m_config->remotes()) {
    if(m_uninstall.count(remote))
      continue;

    const int index = m_list->addRow(makeRow(remote));

    if(find(selected.begin(), selected.end(), remote.name()) != selected.end())
      m_list->select(index);
  }

  m_list->sort();
}

void Manager::setRemoteEnabled(const bool enabled)
{
  for(const int index : m_list->selection()) {
    const Remote &remote = getRemote(index);

    auto it = m_enableOverrides.find(remote);

    if(it == m_enableOverrides.end()) {
      if(remote.isEnabled() != enabled)
        m_enableOverrides.insert({remote, enabled});
    }
    else if(remote.isEnabled() == enabled)
      m_enableOverrides.erase(it);
    else
      it->second = enabled;

    m_list->replaceRow(index, makeRow(remote));
  }

  enable(m_apply);
}

bool Manager::isRemoteEnabled(const Remote &remote) const
{
  const auto it = m_enableOverrides.find(remote);

  if(it == m_enableOverrides.end())
    return remote.isEnabled();
  else
    return it->second;
}

void Manager::uninstall()
{
  int keep = 0;

  while(m_list->selectionSize() - keep > 0) {
    const int index = m_list->currentIndex() + keep;
    const Remote &remote = getRemote(index);

    if(remote.isProtected()) {
      keep++;
      continue;
    }

    m_uninstall.insert(remote);
    enable(m_apply);

    const auto it = m_enableOverrides.find(remote);

    if(it != m_enableOverrides.end())
      m_enableOverrides.erase(it);

    m_list->removeRow(index);
  }
}

void Manager::about(const int index)
{
  m_reapack->about(getRemote(index), handle());
}

void Manager::options()
{
  RECT rect;
  GetWindowRect(getControl(IDC_OPTIONS), &rect);

  Menu menu;

  UINT index = menu.addAction(
    AUTO_STR("&Install new packages automatically"), ACTION_AUTOINSTALL);
  if(m_autoInstall.value_or(m_config->install()->autoInstall))
    menu.check(index);

  index = menu.addAction(
    AUTO_STR("Enable &pre-releases (bleeding edge mode)"), ACTION_BLEEDINGEDGE);
  if(m_bleedingEdge.value_or(m_config->install()->bleedingEdge))
    menu.check(index);

  menu.show(rect.left, rect.bottom - 1, handle());
}

bool Manager::confirm() const
{
  if(m_uninstall.empty())
    return true;

  const size_t uninstallSize = m_uninstall.size();

  auto_char msg[255] = {};
  auto_snprintf(msg, auto_size(msg),
    AUTO_STR("Uninstall %zu repositories%s?\n")
    AUTO_STR("Every file they contain will be removed from your computer."),
    uninstallSize, uninstallSize == 1 ? AUTO_STR("") : AUTO_STR("s"));

  const auto_char *title = AUTO_STR("ReaPack Query");
  const int btn = MessageBox(handle(), msg, title, MB_YESNO);

  return btn == IDYES;
}

bool Manager::apply()
{
  Transaction *tx = m_reapack->setupTransaction();

  if(!tx)
    return false;

  if(m_autoInstall)
    m_config->install()->autoInstall = m_autoInstall.value();

  if(m_bleedingEdge)
    m_config->install()->bleedingEdge = m_bleedingEdge.value();

  for(const auto &pair : m_enableOverrides) {
    const Remote &remote = pair.first;
    const bool enable = pair.second;

    m_reapack->setRemoteEnabled(enable, remote);
  }

  for(const Remote &remote : m_uninstall)
    m_reapack->uninstall(remote);

  tx->runTasks();
  m_config->write();
  reset();

  return true;
}

void Manager::reset()
{
  m_enableOverrides.clear();
  m_uninstall.clear();

  disable(m_apply);
}

ListView::Row Manager::makeRow(const Remote &remote) const
{
  const auto_string &name = make_autostring(remote.name());
  const auto_string &url = make_autostring(remote.url());

  return {name, url, isRemoteEnabled(remote) ?
    AUTO_STR("Enabled") : AUTO_STR("Disabled")};
}

Remote Manager::getRemote(const int index) const
{
  if(index < 0 || index > m_list->rowCount() - 1)
    return {};

  const ListView::Row &row = m_list->row(index);
  const string &remoteName = from_autostring(row[0]);

  return m_config->remotes()->get(remoteName);
}
