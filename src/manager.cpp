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

#include "manager.hpp"

#include "about.hpp"
#include "archive.hpp"
#include "config.hpp"
#include "encoding.hpp"
#include "errors.hpp"
#include "filedialog.hpp"
#include "import.hpp"
#include "menu.hpp"
#include "progress.hpp"
#include "reapack.hpp"
#include "remote.hpp"
#include "resource.hpp"
#include "transaction.hpp"

static const auto_char *ARCHIVE_FILTER =
  AUTO_STR("ReaPack Offline Archive (*.ReaPackArchive)\0*.ReaPackArchive\0");
static const auto_char *ARCHIVE_EXT = AUTO_STR("ReaPackArchive");

using namespace std;

enum {
  ACTION_ENABLE = 80, ACTION_DISABLE, ACTION_UNINSTALL, ACTION_ABOUT,
  ACTION_REFRESH, ACTION_COPYURL, ACTION_SELECT, ACTION_UNSELECT,
  ACTION_AUTOINSTALL_GLOBAL, ACTION_AUTOINSTALL_OFF, ACTION_AUTOINSTALL_ON,
  ACTION_AUTOINSTALL, ACTION_BLEEDINGEDGE, ACTION_PROMPTOBSOLETE,
  ACTION_NETCONFIG, ACTION_RESETCONFIG, ACTION_IMPORT_REPO,
  ACTION_IMPORT_ARCHIVE, ACTION_EXPORT_ARCHIVE
};

Manager::Manager(ReaPack *reapack)
  : Dialog(IDD_CONFIG_DIALOG),
    m_reapack(reapack), m_config(reapack->config()), m_list(nullptr),
    m_changes(0), m_importing(false)
{
}

void Manager::onInit()
{
  Dialog::onInit();

  m_apply = getControl(IDAPPLY);
  disable(m_apply);

  m_list = createControl<ListView>(IDC_LIST, ListView::Columns{
    {AUTO_STR("Name"), 115},
    {AUTO_STR("Index URL"), 415},
    {AUTO_STR("State"), 60},
  });

  m_list->onActivate([=] { m_reapack->about(getRemote(m_list->currentIndex())); });
  m_list->onContextMenu(bind(&Manager::fillContextMenu,
    this, placeholders::_1, placeholders::_2));

  setAnchor(m_list->handle(), AnchorRight | AnchorBottom);
  setAnchor(getControl(IDC_IMPORT), AnchorTop | AnchorBottom);
  setAnchor(getControl(IDC_BROWSE), AnchorTop | AnchorBottom);
  setAnchor(getControl(IDC_OPTIONS), AnchorTop | AnchorBottom);
  setAnchor(getControl(IDOK), AnchorAll);
  setAnchor(getControl(IDCANCEL), AnchorAll);
  setAnchor(m_apply, AnchorAll);

  auto data = m_serializer.read(m_reapack->config()->windowState.manager, 1);
  restoreState(data);
  m_list->restoreState(data);

  refresh();

  m_list->autoSizeHeader();
}

void Manager::onClose()
{
  Serializer::Data data;
  saveState(data);
  m_list->saveState(data);
  m_reapack->config()->windowState.manager = m_serializer.write(data);
}

void Manager::onCommand(const int id, int)
{
  switch(id) {
  case IDC_IMPORT:
    importExport();
    break;
  case IDC_BROWSE:
    launchBrowser();
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
  case ACTION_REFRESH:
    refreshIndex();
    break;
  case ACTION_AUTOINSTALL_GLOBAL:
    setRemoteAutoInstall(boost::logic::indeterminate);
    break;
  case ACTION_AUTOINSTALL_OFF:
    setRemoteAutoInstall(false);
    break;
  case ACTION_AUTOINSTALL_ON:
    setRemoteAutoInstall(true);
    break;
  case ACTION_COPYURL:
    copyUrl();
    break;
  case ACTION_UNINSTALL:
    uninstall();
    break;
  case ACTION_IMPORT_REPO:
    importRepo();
    break;
  case ACTION_IMPORT_ARCHIVE:
    importArchive();
    break;
  case ACTION_EXPORT_ARCHIVE:
    exportArchive();
    break;
  case ACTION_AUTOINSTALL:
    toggle(m_autoInstall, m_config->install.autoInstall);
    break;
  case ACTION_BLEEDINGEDGE:
    toggle(m_bleedingEdge, m_config->install.bleedingEdge);
    break;
  case ACTION_PROMPTOBSOLETE:
    toggle(m_promptObsolete, m_config->install.promptObsolete);
    break;
  case ACTION_NETCONFIG:
    setupNetwork();
    break;
  case ACTION_RESETCONFIG:
    m_config->resetOptions();
    m_config->restoreDefaultRemotes();
    refresh();
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
      setChange(-(int)m_uninstall.size());
      m_uninstall.clear();
      refresh();
      break;
    }
  case IDCANCEL:
    close();
    break;
  default:
    const int action = id >> 8;
    if(action == ACTION_ABOUT)
      m_reapack->about(getRemote(id & 0xff));
    break;
  }
}

bool Manager::fillContextMenu(Menu &menu, const int index) const
{
  const Remote &remote = getRemote(index);

  if(!remote) {
    menu.addAction(AUTO_STR("&Select all"), ACTION_SELECT);
    menu.addAction(AUTO_STR("&Unselect all"), ACTION_UNSELECT);
    return true;
  }

  const UINT enableAction =
    menu.addAction(AUTO_STR("&Enable"), ACTION_ENABLE);
  const UINT disableAction =
    menu.addAction(AUTO_STR("&Disable"), ACTION_DISABLE);

  menu.addSeparator();

  menu.addAction(AUTO_STR("&Refresh"), ACTION_REFRESH);

  Menu autoInstallMenu = menu.addMenu(AUTO_STR("&Install new packages"));
  const UINT autoInstallGlobal = autoInstallMenu.addAction(
    AUTO_STR("Use &global setting"), ACTION_AUTOINSTALL_GLOBAL);
  const UINT autoInstallOff = autoInstallMenu.addAction(
    AUTO_STR("Manually"), ACTION_AUTOINSTALL_OFF);
  const UINT autoInstallOn = autoInstallMenu.addAction(
    AUTO_STR("When synchronizing"), ACTION_AUTOINSTALL_ON);

  menu.addAction(AUTO_STR("&Copy URL"), ACTION_COPYURL);

  const UINT uninstallAction =
    menu.addAction(AUTO_STR("&Uninstall"), ACTION_UNINSTALL);

  menu.addSeparator();

  auto_char aboutLabel[35];
  const auto_string &name = make_autostring(remote.name());
  auto_snprintf(aboutLabel, auto_size(aboutLabel),
    AUTO_STR("&About %s..."), name.c_str());
  menu.addAction(aboutLabel, index | (ACTION_ABOUT << 8));

  bool allEnabled = true;
  bool allDisabled = true;
  bool allProtected = true;
  bool allAutoInstallGlobal = true;
  bool allAutoInstallOff = true;
  bool allAutoInstallOn = true;

  for(const int i : m_list->selection()) {
    const Remote &r = getRemote(i);
    if(isRemoteEnabled(r))
      allDisabled = false;
    else
      allEnabled = false;

    if(!r.isProtected())
      allProtected = false;

    const tribool &autoInstall = remoteAutoInstall(r);
    if(boost::logic::indeterminate(autoInstall)) {
      allAutoInstallOff = false;
      allAutoInstallOn = false;
    }
    else {
      allAutoInstallGlobal = false;
      if(autoInstall)
        allAutoInstallOff = false;
      else if(!autoInstall)
        allAutoInstallOn = false;
    }
  };

  if(allEnabled)
    menu.disable(enableAction);
  if(allDisabled)
    menu.disable(disableAction);
  if(allProtected)
    menu.disable(uninstallAction);

  if(allAutoInstallGlobal)
    autoInstallMenu.check(autoInstallGlobal);
  else if(allAutoInstallOff)
    autoInstallMenu.check(autoInstallOff);
  else if(allAutoInstallOn)
    autoInstallMenu.check(autoInstallOn);

  return true;
}

bool Manager::onKeyDown(const int key, const int mods)
{
  if(GetFocus() != m_list->handle())
    return false;

  if(mods == CtrlModifier && key == 'A')
    m_list->selectAll();
  else if(mods == (CtrlModifier | ShiftModifier) && key == 'A')
    m_list->unselectAll();
  else if(mods == CtrlModifier && key == 'C')
    copyUrl();
  else
    return false;

  return true;
}

void Manager::refresh()
{
  InhibitControl lock(m_list);

  const vector<int> selection = m_list->selection();
  vector<string> selected(selection.size());
  for(size_t i = 0; i < selection.size(); i++)
    selected[i] = from_autostring(m_list->row(selection[i])[0]);

  m_list->clear();

  for(const Remote &remote : m_config->remotes) {
    if(m_uninstall.count(remote))
      continue;

    const int index = m_list->addRow(makeRow(remote));

    if(find(selected.begin(), selected.end(), remote.name()) != selected.end())
      m_list->select(index);
  }

  m_list->sort();
}

void Manager::setMods(const ModsCallback &cb, const bool updateRow)
{
  for(const int index : m_list->selection()) {
    const Remote &remote = getRemote(index);

    auto it = m_mods.find(remote);

    if(it == m_mods.end()) {
      RemoteMods mods;
      cb(remote, &mods);

      if(!mods)
        continue;

      m_mods.insert({remote, mods});
      setChange(1);
    }
    else {
      RemoteMods *mods = &it->second;
      cb(remote, mods);

      if(!*mods) {
        m_mods.erase(it);
        setChange(-1);
      }
    }

    if(updateRow)
      m_list->replaceRow(index, makeRow(remote));
  }
}

void Manager::setRemoteEnabled(const bool enabled)
{
  setMods([=](const Remote &remote, RemoteMods *mods) {
    if(remote.isEnabled() == enabled)
      mods->enable = boost::none;
    else
      mods->enable = enabled;
  }, true);
}

bool Manager::isRemoteEnabled(const Remote &remote) const
{
  const auto it = m_mods.find(remote);

  if(it == m_mods.end())
    return remote.isEnabled();
  else
    return it->second.enable.value_or(remote.isEnabled());
}

void Manager::setRemoteAutoInstall(const tribool &enabled)
{
  setMods([=](const Remote &remote, RemoteMods *mods) {
    const bool same = remote.autoInstall() == enabled
      || (indeterminate(remote.autoInstall()) && indeterminate(enabled));

    if(same)
      mods->autoInstall = boost::none;
    else
      mods->autoInstall = enabled;
  }, true);
}

tribool Manager::remoteAutoInstall(const Remote &remote) const
{
  const auto it = m_mods.find(remote);

  if(it == m_mods.end())
    return remote.autoInstall();
  else
    return it->second.autoInstall.value_or(remote.autoInstall());
}

void Manager::refreshIndex()
{
  if(!m_list->hasSelection())
    return;

  const vector<int> selection = m_list->selection();
  vector<Remote> remotes(selection.size());
  for(size_t i = 0; i < selection.size(); i++)
    remotes[i] = getRemote(selection[i]);

  if(Transaction *tx = m_reapack->setupTransaction())
    tx->fetchIndexes(remotes, true);
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
    setChange(1);

    m_list->removeRow(index);
  }
}

void Manager::toggle(boost::optional<bool> &setting, const bool current)
{
  setting = !setting.value_or(current);
  setChange(*setting == current ? -1 : 1);
}

void Manager::setChange(const int increment)
{
  if(!m_changes && increment < 0)
    return;

  m_changes += increment;

  if(m_changes)
    enable(m_apply);
  else
    disable(m_apply);
}

void Manager::copyUrl()
{
  vector<string> values;

  for(const int index : m_list->selection(false))
    values.push_back(getRemote(index).url());

  setClipboard(values);
}

void Manager::importExport()
{
  Menu menu;
  menu.addAction(AUTO_STR("Import a &repository..."), ACTION_IMPORT_REPO);
  menu.addSeparator();
  menu.addAction(AUTO_STR("Import offline archive..."), ACTION_IMPORT_ARCHIVE);
  menu.addAction(AUTO_STR("&Export offline archive..."), ACTION_EXPORT_ARCHIVE);

  menu.show(getControl(IDC_IMPORT), handle());
}

bool Manager::importRepo()
{
  if(m_importing) // avoid opening the import dialog twice on windows
    return true;

  m_importing = true;
  const auto ret = Dialog::Show<Import>(instance(), handle(), m_reapack);
  m_importing = false;

  return ret != 0;
}

void Manager::importArchive()
{
  const auto_char *title = AUTO_STR("Import offline archive");

  const auto_string &path = FileDialog::getOpenFileName(handle(), instance(),
    title, Path::prefixRoot(Path::DATA), ARCHIVE_FILTER, ARCHIVE_EXT);

  if(path.empty())
    return;

  try {
    Archive::import(path, m_reapack);
  }
  catch(const reapack_error &e) {
    const auto_string &desc = make_autostring(e.what());

    auto_char msg[512];
    auto_snprintf(msg, auto_size(msg),
      AUTO_STR("An error occured while reading %s.\r\n\r\n%s."),
      path.c_str(), desc.c_str()
    );

    MessageBox(handle(), msg, title, MB_OK);
  }
}

void Manager::exportArchive()
{
  const auto_char *title = AUTO_STR("Export offline archive");

  const auto_string &path = FileDialog::getSaveFileName(handle(), instance(),
    title, Path::prefixRoot(Path::DATA), ARCHIVE_FILTER, ARCHIVE_EXT);

  if(path.empty())
    return;

  ThreadPool *pool = new ThreadPool;
  Dialog *progress = Dialog::Create<Progress>(instance(), parent(), pool);

  try {
    const size_t count = Archive::create(path, pool, m_reapack);

    const auto finish = [=] {
      Dialog::Destroy(progress);

      auto_char msg[255];
      auto_snprintf(msg, auto_size(msg),
        AUTO_STR("Done! %zu package%s were exported in the archive."),
        count, count == 1 ? AUTO_STR("") : AUTO_STR("s"));
      MessageBox(handle(), msg, title, MB_OK);

      delete pool;
    };

    pool->onDone(finish);

    if(pool->idle())
      finish();
  }
  catch(const reapack_error &e) {
    Dialog::Destroy(progress);
    delete pool;

    const auto_string &desc = make_autostring(e.what());

    auto_char msg[512];
    auto_snprintf(msg, auto_size(msg),
      AUTO_STR("An error occured while writing into %s.\r\n\r\n%s."),
      path.c_str(), desc.c_str()
    );
    MessageBox(handle(), msg, title, MB_OK);
  }

}

void Manager::launchBrowser()
{
  const auto promptApply = [&] {
    const auto_char *title = AUTO_STR("ReaPack Query");
    const auto_char *msg = AUTO_STR("Apply unsaved changes?");
    const int btn = MessageBox(handle(), msg, title, MB_YESNO);
    return btn == IDYES;
  };

  if(m_changes && promptApply())
    apply();

  m_reapack->browsePackages();
}

void Manager::options()
{
  Menu menu;

  UINT index = menu.addAction(
    AUTO_STR("&Install new packages when synchronizing"), ACTION_AUTOINSTALL);
  if(m_autoInstall.value_or(m_config->install.autoInstall))
    menu.check(index);

  index = menu.addAction(
    AUTO_STR("Enable &pre-releases globally (bleeding edge)"), ACTION_BLEEDINGEDGE);
  if(m_bleedingEdge.value_or(m_config->install.bleedingEdge))
    menu.check(index);

  index = menu.addAction(
    AUTO_STR("Prompt to uninstall obsolete packages"), ACTION_PROMPTOBSOLETE);
  if(m_promptObsolete.value_or(m_config->install.promptObsolete))
    menu.check(index);

  menu.addAction(AUTO_STR("&Network settings..."), ACTION_NETCONFIG);

  menu.addSeparator();

  menu.addAction(AUTO_STR("&Restore default settings"), ACTION_RESETCONFIG);

  menu.show(getControl(IDC_OPTIONS), handle());
}

void Manager::setupNetwork()
{
  if(IDOK == Dialog::Show<NetworkConfig>(instance(), handle(), &m_config->network))
    m_config->write();
}

bool Manager::confirm() const
{
  if(m_uninstall.empty())
    return true;

  const size_t uninstallSize = m_uninstall.size();

  auto_char msg[255];
  auto_snprintf(msg, auto_size(msg), AUTO_STR("Uninstall %zu %s?\n")
    AUTO_STR("Every file they contain will be removed from your computer."),
    uninstallSize,
    uninstallSize == 1 ? AUTO_STR("repository") : AUTO_STR("repositories"));

  const auto_char *title = AUTO_STR("ReaPack Query");
  const int btn = MessageBox(handle(), msg, title, MB_YESNO);

  return btn == IDYES;
}

bool Manager::apply()
{
  if(!m_changes)
    return true;

  Transaction *tx = m_reapack->setupTransaction();

  if(!tx)
    return false;

  bool syncAll = false;

  if(m_autoInstall) {
    m_config->install.autoInstall = m_autoInstall.value();
    syncAll = m_autoInstall.value();
  }

  if(m_bleedingEdge)
    m_config->install.bleedingEdge = m_bleedingEdge.value();

  if(m_promptObsolete)
    m_config->install.promptObsolete = m_promptObsolete.value();

  for(const auto &pair : m_mods) {
    Remote remote = pair.first;
    const RemoteMods &mods = pair.second;

    if(m_uninstall.find(remote) != m_uninstall.end())
      continue;

    if(mods.enable) {
      m_reapack->setRemoteEnabled(*mods.enable, remote);

      if(*mods.enable && !syncAll)
        tx->synchronize(remote);
    }
    if(mods.autoInstall) {
      remote.setAutoInstall(*mods.autoInstall);
      m_config->remotes.add(remote);

      const bool isEnabled = mods.enable.value_or(remote.isEnabled());
      if(isEnabled && *mods.autoInstall && !syncAll)
        tx->synchronize(remote);
    }
  }

  for(const Remote &remote : m_uninstall)
    m_reapack->uninstall(remote);

  if(syncAll)
    m_reapack->synchronizeAll();

  tx->runTasks();
  m_config->write();
  reset();

  return true;
}

void Manager::reset()
{
  m_mods.clear();
  m_uninstall.clear();
  m_autoInstall = boost::none;
  m_bleedingEdge = boost::none;
  m_promptObsolete = boost::none;

  m_changes = 0;
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

  return m_config->remotes.get(remoteName);
}

NetworkConfig::NetworkConfig(NetworkOpts *opts)
  : Dialog(IDD_NETCONF_DIALOG), m_opts(opts)
{
}

void NetworkConfig::onInit()
{
  Dialog::onInit();

  m_proxy = getControl(IDC_PROXY);
  SetWindowText(m_proxy, make_autostring(m_opts->proxy).c_str());

  m_verifyPeer = getControl(IDC_VERIFYPEER);
  SendMessage(m_verifyPeer, BM_SETCHECK,
    m_opts->verifyPeer ? BST_CHECKED : BST_UNCHECKED, 0);
}

void NetworkConfig::onCommand(const int id, int)
{
  switch(id) {
  case IDOK:
    apply();
    // then close
  case IDCANCEL:
    close(id);
    break;
  }
}

void NetworkConfig::apply()
{
  m_opts->proxy = getText(m_proxy);
  m_opts->verifyPeer = SendMessage(m_verifyPeer, BM_GETCHECK, 0, 0) == BST_CHECKED;
}
