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
#include "errors.hpp"
#include "filedialog.hpp"
#include "import.hpp"
#include "listview.hpp"
#include "menu.hpp"
#include "progress.hpp"
#include "reapack.hpp"
#include "remote.hpp"
#include "resource.hpp"
#include "transaction.hpp"
#include "win32.hpp"

static const Win32::char_type *ARCHIVE_FILTER =
  L("ReaPack Offline Archive (*.ReaPackArchive)\0*.ReaPackArchive\0");
static const Win32::char_type *ARCHIVE_EXT = L("ReaPackArchive");

using namespace std;

enum {
  ACTION_UNINSTALL = 80, ACTION_ABOUT, ACTION_REFRESH, ACTION_COPYURL,
  ACTION_SELECT, ACTION_UNSELECT, ACTION_AUTOINSTALL_GLOBAL,
  ACTION_AUTOINSTALL_OFF, ACTION_AUTOINSTALL_ON, ACTION_AUTOINSTALL,
  ACTION_BLEEDINGEDGE, ACTION_PROMPTOBSOLETE, ACTION_NETCONFIG,
  ACTION_RESETCONFIG, ACTION_IMPORT_REPO, ACTION_IMPORT_ARCHIVE,
  ACTION_EXPORT_ARCHIVE,
};

Manager::Manager()
  : Dialog(IDD_CONFIG_DIALOG),
    m_list(nullptr), m_changes(0), m_importing(false)
{
}

void Manager::onInit()
{
  Dialog::onInit();

  auto msize = minimumSize();
  msize.y = 210;
  setMinimumSize(msize);

  m_apply = getControl(IDAPPLY);
  disable(m_apply);

  m_list = createControl<ListView>(IDC_LIST, ListView::Columns{
    {"Name", 145},
    {"Index URL", 445},
  });

  m_list->enableIcons();
  m_list->onSelect(bind(&Dialog::startTimer, this, 100, 0, true));
  m_list->onIconClick(bind(&Manager::toggleEnabled, this));
  m_list->onActivate(bind(&Manager::aboutRepo, this, true));
  m_list->onContextMenu(bind(&Manager::fillContextMenu, this, _1, _2));

  setAnchor(m_list->handle(), AnchorRight | AnchorBottom);
  setAnchor(getControl(IDC_IMPORT), AnchorTop | AnchorBottom);
  setAnchor(getControl(IDC_BROWSE), AnchorTop | AnchorBottom);
  setAnchor(getControl(IDC_OPTIONS), AnchorTop | AnchorBottom);
  setAnchor(getControl(IDOK), AnchorAll);
  setAnchor(getControl(IDCANCEL), AnchorAll);
  setAnchor(m_apply, AnchorAll);

  auto data = m_serializer.read(g_reapack->config()->windowState.manager, 1);
  restoreState(data);
  m_list->restoreState(data);

  refresh();

  m_list->autoSizeHeader();
}

void Manager::onTimer(const int id)
{
  stopTimer(id);

  if(About *about = g_reapack->about(false)) {
    if(about->testDelegate<AboutIndexDelegate>())
      aboutRepo(false);
  }
}

void Manager::onClose()
{
  Serializer::Data data;
  saveState(data);
  m_list->saveState(data);
  g_reapack->config()->windowState.manager = m_serializer.write(data);
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
    toggle(m_autoInstall, g_reapack->config()->install.autoInstall);
    break;
  case ACTION_BLEEDINGEDGE:
    toggle(m_bleedingEdge, g_reapack->config()->install.bleedingEdge);
    break;
  case ACTION_PROMPTOBSOLETE:
    toggle(m_promptObsolete, g_reapack->config()->install.promptObsolete);
    break;
  case ACTION_NETCONFIG:
    setupNetwork();
    break;
  case ACTION_RESETCONFIG:
    g_reapack->config()->resetOptions();
    g_reapack->config()->restoreDefaultRemotes();
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
    // FALLTHRU
  case IDCANCEL:
    close();
    break;
  default:
    const int action = id >> 8;
    if(action == ACTION_ABOUT)
      g_reapack->about(getRemote(id & 0xff));
    break;
  }
}

bool Manager::fillContextMenu(Menu &menu, const int index) const
{
  const Remote &remote = getRemote(index);

  if(!remote) {
    menu.addAction("&Select all", ACTION_SELECT);
    menu.addAction("&Unselect all", ACTION_UNSELECT);
    return true;
  }

  menu.addAction("&Refresh", ACTION_REFRESH);
  menu.addAction("&Copy URL", ACTION_COPYURL);

  Menu autoInstallMenu = menu.addMenu("&Install new packages");
  const UINT autoInstallGlobal = autoInstallMenu.addAction(
    "Use &global setting", ACTION_AUTOINSTALL_GLOBAL);
  const UINT autoInstallOff = autoInstallMenu.addAction(
    "Manually", ACTION_AUTOINSTALL_OFF);
  const UINT autoInstallOn = autoInstallMenu.addAction(
    "When synchronizing", ACTION_AUTOINSTALL_ON);

  const UINT uninstallAction =
    menu.addAction("&Uninstall", ACTION_UNINSTALL);

  menu.addSeparator();

  menu.addAction(String::format("&About %s", remote.name().c_str()),
    index | (ACTION_ABOUT << 8));

  bool allProtected = true;
  bool allAutoInstallGlobal = true;
  bool allAutoInstallOff = true;
  bool allAutoInstallOn = true;

  for(const int i : m_list->selection()) {
    const Remote &r = getRemote(i);
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
  ListView::BeginEdit edit(m_list);

  const vector<int> selection = m_list->selection();
  vector<string> selected(selection.size());
  for(size_t i = 0; i < selection.size(); i++)
    selected[i] = m_list->row(selection[i])->cell(0).value; // TODO: use data ptr to Remote

  const auto &remotes = g_reapack->config()->remotes;

  m_list->clear();
  m_list->reserveRows(remotes.size());

  for(const Remote &remote : remotes) {
    if(m_uninstall.count(remote))
      continue;

    int c = 0;
    auto row = m_list->createRow();
    row->setChecked(isRemoteEnabled(remote));
    row->setCell(c++, remote.name());
    row->setCell(c++, remote.url());

    if(find(selected.begin(), selected.end(), remote.name()) != selected.end())
      m_list->select(row->index());
  }
}

void Manager::setMods(const ModsCallback &cb, const bool updateRow)
{
  ListView::BeginEdit edit(m_list);

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
      m_list->row(index)->setChecked(isRemoteEnabled(remote)); // TODO: move into cb
  }
}

void Manager::toggleEnabled()
{
  setMods([=](const Remote &remote, RemoteMods *mods) {
    const bool enabled = !mods->enable.value_or(remote.isEnabled());

    if(remote.isEnabled() == enabled)
      mods->enable = boost::none;
    else
      mods->enable = enabled;
  }, true);
}

bool Manager::isRemoteEnabled(const Remote &remote) const
{
  const auto &it = m_mods.find(remote);

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
  }, false);
}

tribool Manager::remoteAutoInstall(const Remote &remote) const
{
  const auto &it = m_mods.find(remote);

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

  if(Transaction *tx = g_reapack->setupTransaction())
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

void Manager::aboutRepo(const bool focus)
{
  if(m_list->hasSelection())
    g_reapack->about(getRemote(m_list->currentIndex()), focus);
}

void Manager::importExport()
{
  Menu menu;
  menu.addAction("Import &repositories...", ACTION_IMPORT_REPO);
  menu.addSeparator();
  menu.addAction("Import offline archive...", ACTION_IMPORT_ARCHIVE);
  menu.addAction("&Export offline archive...", ACTION_EXPORT_ARCHIVE);

  menu.show(getControl(IDC_IMPORT), handle());
}

bool Manager::importRepo()
{
  if(m_importing) // avoid opening the import dialog twice on windows
    return true;

  m_importing = true;
  const auto ret = Dialog::Show<Import>(instance(), handle());
  m_importing = false;

  return ret != 0;
}

void Manager::importArchive()
{
  const char *title = "Import offline archive";

  const string &path = FileDialog::getOpenFileName(handle(), instance(),
    title, Path::DATA.prependRoot(), ARCHIVE_FILTER, ARCHIVE_EXT);

  if(path.empty())
    return;

  try {
    Archive::import(path);
  }
  catch(const reapack_error &e) {
    char msg[512];
    snprintf(msg, sizeof(msg), "An error occured while reading %s.\n\n%s.",
      path.c_str(), e.what());
    Win32::messageBox(handle(), msg, title, MB_OK);
  }
}

void Manager::exportArchive()
{
  const string &path = FileDialog::getSaveFileName(handle(), instance(),
    "Export offline archive", Path::DATA.prependRoot(), ARCHIVE_FILTER, ARCHIVE_EXT);

  if(!path.empty()) {
    if(Transaction *tx = g_reapack->setupTransaction()) {
      tx->exportArchive(path);
      tx->runTasks();
    }
  }
}

void Manager::launchBrowser()
{
  const auto promptApply = [this] {
    return IDYES == Win32::messageBox(handle(), "Apply unsaved changes?", "ReaPack Query", MB_YESNO);
  };

  if(m_changes && promptApply())
    apply();

  g_reapack->browsePackages();
}

void Manager::options()
{
  Menu menu;

  UINT index = menu.addAction(
    "&Install new packages when synchronizing", ACTION_AUTOINSTALL);
  if(m_autoInstall.value_or(g_reapack->config()->install.autoInstall))
    menu.check(index);

  index = menu.addAction(
    "Enable &pre-releases globally (bleeding edge)", ACTION_BLEEDINGEDGE);
  if(m_bleedingEdge.value_or(g_reapack->config()->install.bleedingEdge))
    menu.check(index);

  index = menu.addAction(
    "Prompt to uninstall obsolete packages", ACTION_PROMPTOBSOLETE);
  if(m_promptObsolete.value_or(g_reapack->config()->install.promptObsolete))
    menu.check(index);

  menu.addAction("&Network settings...", ACTION_NETCONFIG);

  menu.addSeparator();

  menu.addAction("&Restore default settings", ACTION_RESETCONFIG);

  menu.show(getControl(IDC_OPTIONS), handle());
}

void Manager::setupNetwork()
{
  if(IDOK == Dialog::Show<NetworkConfig>(instance(), handle(), &g_reapack->config()->network))
    g_reapack->config()->write();
}

bool Manager::confirm() const
{
  if(m_uninstall.empty())
    return true;

  const size_t uninstallSize = m_uninstall.size();

  char msg[255];
  snprintf(msg, sizeof(msg), "Uninstall %zu %s?\n"
    "Every file they contain will be removed from your computer.",
    uninstallSize, uninstallSize == 1 ? "repository" : "repositories");

  return IDYES == Win32::messageBox(handle(), msg, "ReaPack Query", MB_YESNO);
}

bool Manager::apply()
{
  if(!m_changes)
    return true;

  Transaction *tx = g_reapack->setupTransaction();

  if(!tx)
    return false;

  // syncList is the list of repos to synchronize for autoinstall
  // (global or local setting)
  set<Remote> syncList;

  if(m_autoInstall) {
    g_reapack->config()->install.autoInstall = m_autoInstall.value();

    if(m_autoInstall.value()) {
      const auto &enabledNow = g_reapack->config()->remotes.getEnabled();
      copy(enabledNow.begin(), enabledNow.end(), inserter(syncList, syncList.end()));
    }
  }

  if(m_bleedingEdge)
    g_reapack->config()->install.bleedingEdge = m_bleedingEdge.value();

  if(m_promptObsolete)
    g_reapack->config()->install.promptObsolete = m_promptObsolete.value();

  for(const auto &pair : m_mods) {
    Remote remote = pair.first;
    const RemoteMods &mods = pair.second;

    if(m_uninstall.find(remote) != m_uninstall.end())
      continue;

    if(mods.enable) {
      remote.setEnabled(*mods.enable);
      syncList.erase(remote);
    }

    if(mods.autoInstall) {
      remote.setAutoInstall(*mods.autoInstall);

      const bool isEnabled = mods.enable.value_or(remote.isEnabled());

      if(isEnabled && *mods.autoInstall)
        syncList.insert(remote);
    }

    g_reapack->addSetRemote(remote);
  }

  for(const Remote &remote : m_uninstall) {
    g_reapack->uninstall(remote);
    syncList.erase(remote);
  }

  for(const Remote &remote : syncList)
    tx->synchronize(remote);

  reset();

  g_reapack->commitConfig();

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

Remote Manager::getRemote(const int index) const
{
  if(index < 0 || index > m_list->rowCount() - 1)
    return {};

  const string &remoteName = m_list->row(index)->cell(0).value;
  return g_reapack->config()->remotes.get(remoteName);
}

NetworkConfig::NetworkConfig(NetworkOpts *opts)
  : Dialog(IDD_NETCONF_DIALOG), m_opts(opts)
{
}

void NetworkConfig::onInit()
{
  Dialog::onInit();

  m_proxy = getControl(IDC_PROXY);
  Win32::setWindowText(m_proxy, m_opts->proxy.c_str());

  m_verifyPeer = getControl(IDC_VERIFYPEER);
  setChecked(m_opts->verifyPeer, m_verifyPeer);

  m_staleThreshold = getControl(IDC_STALETHRSH);
  setChecked(m_opts->staleThreshold > 0, m_staleThreshold);
}

void NetworkConfig::onCommand(const int id, int)
{
  switch(id) {
  case IDOK:
    apply();
    // FALLTHROUGH
  case IDCANCEL:
    close(id);
    break;
  }
}

void NetworkConfig::apply()
{
  m_opts->proxy = Win32::getWindowText(m_proxy);
  m_opts->verifyPeer = isChecked(m_verifyPeer);
  m_opts->staleThreshold = isChecked(m_staleThreshold)
    ? NetworkOpts::OneWeekThreshold : NetworkOpts::NoThreshold;
}
