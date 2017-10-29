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

#include "browser.hpp"

#include "about.hpp"
#include "browser_entry.hpp"
#include "config.hpp"
#include "errors.hpp"
#include "index.hpp"
#include "listview.hpp"
#include "menu.hpp"
#include "reapack.hpp"
#include "resource.hpp"
#include "transaction.hpp"
#include "win32.hpp"

using namespace std;

enum Timers { TIMER_FILTER = 1, TIMER_ABOUT };

Browser::Browser()
  : Dialog(IDD_BROWSER_DIALOG), m_loadState(Init), m_currentIndex(-1)
{
}

void Browser::onInit()
{
  m_applyBtn = getControl(IDAPPLY);
  m_filter = getControl(IDC_FILTER);
  m_view = getControl(IDC_TABS);
  m_displayBtn = getControl(IDC_DISPLAY);
  m_actionsBtn = getControl(IDC_ACTION);

  disable(m_applyBtn);
  disable(m_actionsBtn);

  // don't forget to update order of enum View in header file
  SendMessage(m_view, CB_ADDSTRING, 0, (LPARAM)L("All"));
  SendMessage(m_view, CB_ADDSTRING, 0, (LPARAM)L("Queued"));
  SendMessage(m_view, CB_ADDSTRING, 0, (LPARAM)L("Installed"));
  SendMessage(m_view, CB_ADDSTRING, 0, (LPARAM)L("Out of date"));
  SendMessage(m_view, CB_ADDSTRING, 0, (LPARAM)L("Obsolete"));
  SendMessage(m_view, CB_ADDSTRING, 0, (LPARAM)L("Uninstalled"));
  SendMessage(m_view, CB_SETCURSEL, 0, 0);

  m_list = createControl<ListView>(IDC_LIST, ListView::Columns{
    {"Status",       23, ListView::NoLabelFlag},
    {"Package",     345, ListView::FilterFlag},
    {"Category",    105, ListView::FilterFlag},
    {"Version",      55, 0, ListView::VersionType},
    {"Author",       95, ListView::FilterFlag},
    {"Type",         70},
    {"Repository",  120, ListView::CollapseFlag | ListView::FilterFlag},
    {"Last Update", 105, 0, ListView::TimeType},
  });

  m_list->onActivate([=] { aboutPackage(m_list->itemUnderMouse()); });
  m_list->onSelect(bind(&Browser::onSelection, this));
  m_list->onContextMenu(bind(&Browser::fillContextMenu, this, _1, _2));
  m_list->sortByColumn(1);

  Dialog::onInit();
  setMinimumSize({600, 250});

  setAnchor(m_filter, AnchorRight);
  setAnchor(getControl(IDC_CLEAR), AnchorLeftRight);
  setAnchor(getControl(IDC_LABEL2), AnchorLeftRight);
  setAnchor(m_view, AnchorLeftRight);
  setAnchor(m_displayBtn, AnchorLeftRight);
  setAnchor(m_list->handle(), AnchorRight | AnchorBottom);
  setAnchor(getControl(IDC_SELECT), AnchorTop | AnchorBottom);
  setAnchor(getControl(IDC_UNSELECT), AnchorTop | AnchorBottom);
  setAnchor(m_actionsBtn, AnchorTop | AnchorBottom);
  setAnchor(getControl(IDOK), AnchorAll);
  setAnchor(getControl(IDCANCEL), AnchorAll);
  setAnchor(m_applyBtn, AnchorAll);

  auto data = m_serializer.read(g_reapack->config()->windowState.browser, 1);
  restoreState(data);
  m_list->restoreState(data);

  updateDisplayLabel();
}

void Browser::onClose()
{
  Serializer::Data data;
  saveState(data);
  m_list->saveState(data);
  g_reapack->config()->windowState.browser = m_serializer.write(data);
}

void Browser::onCommand(const int id, const int event)
{
  switch(id) {
  case IDC_TABS:
    if(event == CBN_SELCHANGE)
      fillList();
    break;
  case IDC_DISPLAY:
    displayButton();
    break;
  case IDC_FILTER:
    if(event == EN_CHANGE)
      startTimer(200, TIMER_FILTER, false);
    break;
  case IDC_CLEAR:
    setFilter({});
    break;
  case IDC_SELECT:
    m_list->selectAll();
    SetFocus(m_list->handle());
    break;
  case IDC_UNSELECT:
    m_list->unselectAll();
    SetFocus(m_list->handle());
    break;
  case IDC_ACTION:
    actionsButton();
    break;
  case ACTION_LATEST:
    currentDo(bind(&Browser::installLatest, this, _1, true));
    break;
  case ACTION_LATEST_ALL:
    installLatestAll();
    break;
  case ACTION_REINSTALL:
    currentDo(bind(&Browser::reinstall, this, _1, true));
    break;
  case ACTION_REINSTALL_ALL:
    selectionDo(bind(&Browser::reinstall, this, _1, false));
    break;
  case ACTION_UNINSTALL:
    currentDo(bind(&Browser::uninstall, this, _1, true));
    break;
  case ACTION_UNINSTALL_ALL:
    selectionDo(bind(&Browser::uninstall, this, _1, false));
    break;
  case ACTION_PIN:
    currentDo(bind(&Browser::togglePin, this, _1));
    break;
  case ACTION_ABOUT_PKG:
    aboutPackage(m_currentIndex);
    break;
  case ACTION_ABOUT_REMOTE:
    aboutRemote(m_currentIndex);
    break;
  case ACTION_RESET_ALL:
    selectionDo(bind(&Browser::resetActions, this, _1));
    break;
  case ACTION_COPY:
    copy();
    break;
  case ACTION_REFRESH:
    refresh(true);
    break;
  case ACTION_MANAGE:
    g_reapack->manageRemotes();
    break;
  case ACTION_FILTERTYPE:
    m_typeFilter = boost::none;
    fillList();
    break;
  case IDOK:
  case IDAPPLY:
    if(confirm()) {
      if(!apply() || id == IDAPPLY)
        break;
    }
    else
      break;
  case IDCANCEL:
    if(m_loadState == Loading)
      hide(); // keep ourselves alive
    else
      close();
    break;
  default:
    if(id >> 8 == ACTION_VERSION)
      currentDo(bind(&Browser::installVersion, this, _1, id & 0xff));
    else if(id >> 8 == ACTION_FILTERTYPE) {
      m_typeFilter = static_cast<Package::Type>(id & 0xff);
      fillList();
    }
    break;
  }
}

bool Browser::onKeyDown(const int key, const int mods)
{
  if(GetFocus() != m_list->handle()) {
    if(key == VK_UP || key == VK_DOWN)
      SetFocus(m_list->handle());

    return false;
  }

  if(mods == CtrlModifier && key == 'A')
    m_list->selectAll();
  else if(mods == (CtrlModifier | ShiftModifier) && key == 'A')
    m_list->unselectAll();
  else if(mods == CtrlModifier && key == 'C')
    copy();
  else if(!mods && key == VK_F5)
    refresh(true);
  else if(!mods && key == VK_SPACE)
    aboutPackage(m_list->currentIndex());
  else
    return false;

  return true;
}

void Browser::onTimer(const int id)
{
  switch(id) {
  case TIMER_FILTER:
    updateFilter();
    break;
  case TIMER_ABOUT:
    updateAbout();
    break;
  }
}

void Browser::onSelection()
{
  setEnabled(m_list->hasSelection(), m_actionsBtn);
  startTimer(100, TIMER_ABOUT);
}

bool Browser::fillContextMenu(Menu &menu, const int index)
{
  m_currentIndex = index;
  fillMenu(menu);

  menu.addAction("&Select all", IDC_SELECT);
  menu.addAction("&Unselect all", IDC_UNSELECT);

  return true;
}

void Browser::fillMenu(Menu &menu)
{
  const Entry *entry = getEntry(m_currentIndex);

  if(m_list->selectionSize() > 1) {
    fillSelectionMenu(menu);
    menu.addSeparator();

    if(entry) {
      Menu pkgMenu = menu.addMenu("Package under cursor");
      entry->fillMenu(pkgMenu);
    }
  }
  else if(entry)
    entry->fillMenu(menu);

  if(!menu.empty())
    menu.addSeparator();

  if(m_list->hasSelection())
    menu.addAction("&Copy package names", ACTION_COPY);
}

void Browser::fillSelectionMenu(Menu &menu)
{
  int selFlags = 0;

  for(const int index : m_list->selection())
    selFlags |= getEntry(index)->possibleActions();

  menu.setEnabled(selFlags & Entry::CanInstallLatest,
    menu.addAction("&Install/update selection", ACTION_LATEST_ALL));
  menu.setEnabled(selFlags & Entry::CanReinstall,
    menu.addAction("&Reinstall selection", ACTION_REINSTALL_ALL));
  menu.setEnabled(selFlags & Entry::CanUninstall,
    menu.addAction("&Uninstall selection", ACTION_UNINSTALL_ALL));
  menu.setEnabled(selFlags & Entry::CanClearQueued,
    menu.addAction("&Clear queued actions", ACTION_RESET_ALL));
}

void Browser::updateDisplayLabel()
{
  char btnLabel[32];
  snprintf(btnLabel, sizeof(btnLabel), "%d/%zu package%s...",
    m_list->visibleRowCount(), m_entries.size(),
    m_entries.size() == 1 ? "" : "s");

  Win32::setWindowText(m_displayBtn, btnLabel);
}

void Browser::displayButton()
{
  static map<const char *, Package::Type> types = {
    {"&Scripts", Package::ScriptType},
    {"&Effects", Package::EffectType},
    {"E&xtensions", Package::ExtensionType},
    {"&Themes", Package::ThemeType},
    {"&Language Packs", Package::LangPackType},
    {"&Web Interfaces", Package::WebInterfaceType},
    {"&Project Templates", Package::ProjectTemplateType},
    {"&Track Templates", Package::TrackTemplateType},
    {"&MIDI Note Names", Package::MIDINoteNamesType},
    {"&Automation Items", Package::AutomationItemType},
    {"&Other packages", Package::UnknownType},
  };

  Menu menu;

  auto index = menu.addAction("&All packages", ACTION_FILTERTYPE);
  if(!m_typeFilter)
    menu.checkRadio(index);

  for(const auto &pair : types) {
    auto index = menu.addAction(pair.first,
      pair.second | (ACTION_FILTERTYPE << 8));

    if(m_typeFilter && m_typeFilter == pair.second)
      menu.checkRadio(index);
  }

  menu.addSeparator();

  menu.addAction("&Refresh repositories", ACTION_REFRESH);
  menu.addAction("&Manage repositories...", ACTION_MANAGE);

  menu.show(m_displayBtn, handle());
}

void Browser::actionsButton()
{
  m_currentIndex = m_list->currentIndex();

  Menu menu;
  fillMenu(menu);
  menu.show(m_actionsBtn, handle());
}

bool Browser::isFiltered(Package::Type type) const
{
  if(!m_typeFilter)
    return false;

  switch(type) {
  case Package::UnknownType:
  case Package::ScriptType:
  case Package::EffectType:
  case Package::ExtensionType:
  case Package::ThemeType:
  case Package::LangPackType:
  case Package::WebInterfaceType:
  case Package::ProjectTemplateType:
  case Package::TrackTemplateType:
  case Package::MIDINoteNamesType:
  case Package::AutomationItemType:
    break;
  case Package::DataType:
    type = Package::UnknownType;
    break;
  }

  return m_typeFilter != type;
}

void Browser::updateFilter()
{
  stopTimer(TIMER_FILTER);

  m_list->setFilter(Win32::getWindowText(m_filter));
  updateDisplayLabel();
}

void Browser::updateAbout()
{
  stopTimer(TIMER_ABOUT);

  About *about = g_reapack->about(false);

  if(!about)
    return;

  const auto index = m_list->currentIndex();

  if(about->testDelegate<AboutIndexDelegate>())
    aboutRemote(index, false);
  else if(about->testDelegate<AboutPackageDelegate>())
    aboutPackage(index, false);
}

void Browser::refresh(const bool stale)
{
  switch(m_loadState) {
  case DeferredLoaded:
    // We already processed this transaction immediately before.
    m_loadState = Loaded;
    return;
  case Loading:
    // Don't refresh again while currently refreshing.
    return;
  default:
    break;
  }

  const vector<Remote> &remotes = g_reapack->config()->remotes.getEnabled();

  if(remotes.empty()) {
    if(!isVisible() || stale) {
      show();

      Win32::messageBox(handle(), "No repository enabled!\n"
        "Enable or import repositories from Extensions > ReaPack > Manage repositories.",
        "Browse packages", MB_OK);
    }

    // Clear the list if it were previously filled.
    populate({}, nullptr);
    return;
  }

  if(Transaction *tx = g_reapack->setupTransaction()) {
    const bool isFirstLoad = m_loadState == Init;
    m_loadState = Loading;

    tx->fetchIndexes(remotes, stale);
    tx->onFinish([=] {
      if(isFirstLoad || isVisible()) {
        populate(tx->getIndexes(remotes), tx->registry());

        // Ignore the next call to refreshBrowser() if we know we'll be
        // requested to handle the very same transaction.
        m_loadState = tx->receipt()->test(Receipt::RefreshBrowser) ?
          DeferredLoaded : Loaded;
      }
      else {
        // User manually asked to refresh the browser but closed the window
        // before it could finished fetching the up to date indexes.
        close();
      }
    });

    tx->runTasks();
  }
}

void Browser::populate(const vector<IndexPtr> &indexes, const Registry *reg)
{
  // keep previous entries in memory a bit longer for #transferActions
  vector<Entry> oldEntries;
  swap(m_entries, oldEntries);

  m_currentIndex = -1;

  for(const IndexPtr &index : indexes) {
    for(const Package *pkg : index->packages())
      m_entries.push_back({pkg, reg->getEntry(pkg), index});

    // obsolete packages
    for(const Registry::Entry &regEntry : reg->getEntries(index->name())) {
      if(!index->find(regEntry.category, regEntry.package))
        m_entries.push_back({regEntry, index});
    }
  }

  transferActions();
  fillList();

  if(!isVisible())
    show();
}

void Browser::setFilter(const string &newFilter)
{
  Win32::setWindowText(m_filter, newFilter.c_str());
  updateFilter(); // don't wait for the timer, update now!
  SetFocus(m_filter);
}

void Browser::transferActions()
{
  list<Entry *> oldActions;
  swap(m_actions, oldActions);

  for(Entry *oldEntry : oldActions) {
    const auto &entryIt = find(m_entries.begin(), m_entries.end(), *oldEntry);
    if(entryIt == m_entries.end())
      continue;

    if(oldEntry->target) {
      const Version *target = *oldEntry->target;

      if(target) {
        const Package *pkg = entryIt->package;
        if(!pkg || !(target = pkg->findVersion(target->name())))
          continue;
      }

      entryIt->target = target;
    }

    if(oldEntry->pin)
      entryIt->pin = *oldEntry->pin;

    m_actions.push_back(&*entryIt);
  }

  if(m_actions.empty())
    disable(m_applyBtn);
}

void Browser::fillList()
{
  ListView::BeginEdit edit(m_list);

  const int scroll = m_list->scroll();

  vector<int> selectIndexes = m_list->selection();
  vector<const Entry *> oldSelection(selectIndexes.size());
  for(size_t i = 0; i < selectIndexes.size(); i++)
    oldSelection[i] = static_cast<Entry *>(m_list->row(selectIndexes[i])->userData);
  selectIndexes.clear(); // will put new indexes below

  m_list->clear();
  m_list->reserveRows(m_entries.size());

  for(const Entry &entry : m_entries) {
    if(!match(entry))
      continue;

    auto row = m_list->createRow((void *)&entry);
    entry.updateRow(row);

    const auto &matchingEntryIt = find_if(oldSelection.begin(), oldSelection.end(),
      [&entry] (const Entry *oldEntry) { return *oldEntry == entry; });

    if(matchingEntryIt != oldSelection.end())
      selectIndexes.push_back(row->index());
  }

  m_list->setScroll(scroll);

  // restore selection only after having sorted the table
  // in order to get the same scroll position as before if possible
  for(const int index : selectIndexes)
    m_list->select(index);

  m_list->endEdit(); // filter before calling updateDisplayLabel
  updateDisplayLabel();
}

bool Browser::match(const Entry &entry) const
{
  if(isFiltered(entry.type()))
    return false;

  switch(currentView()) {
  case AllView:
    break;
  case QueuedView:
    if(!hasAction(&entry))
      return false;
    break;
  case InstalledView:
    if(!entry.test(Entry::InstalledFlag))
      return false;
    break;
  case OutOfDateView:
    if(!entry.test(Entry::OutOfDateFlag))
      return false;
    break;
  case UninstalledView:
    if(!entry.test(Entry::UninstalledFlag))
      return false;
    break;
  case ObsoleteView:
    if(!entry.test(Entry::ObsoleteFlag))
      return false;
    break;
  }

  return true;
}

auto Browser::getEntry(const int index) -> Entry *
{
  if(index < 0)
    return nullptr;
  else
    return static_cast<Entry *>(m_list->row(index)->userData);
}

void Browser::aboutPackage(const int index, const bool focus)
{
  const Entry *entry = getEntry(index);

  if(entry && entry->package) {
    g_reapack->about()->setDelegate(make_shared<AboutPackageDelegate>(
      entry->package, entry->regEntry.version), focus);
  }
}

void Browser::aboutRemote(const int index, const bool focus)
{
  if(const Entry *entry = getEntry(index)) {
    g_reapack->about()->setDelegate(
      make_shared<AboutIndexDelegate>(entry->index), focus);
  }
}

void Browser::installLatestAll()
{
  InstallOpts &installOpts = g_reapack->config()->install;
  const bool isEverything = (size_t)m_list->selectionSize() == m_entries.size();

  if(isEverything && !installOpts.autoInstall) {
    const int btn = Win32::messageBox(handle(), "Do you want ReaPack to install"
      " new packages automatically when synchronizing in the future?\n\nThis"
      " setting can also be customized globally or on a per-repository basis"
      " in ReaPack > Manage repositories.",
      "Install every available packages", MB_YESNOCANCEL);

    switch(btn) {
    case IDYES:
      installOpts.autoInstall = true;
      break;
    case IDCANCEL:
      return;
    }
  }

  selectionDo(bind(&Browser::installLatest, this, _1, false));
}

void Browser::installLatest(const int index, const bool toggle)
{
  const Entry *entry = getEntry(index);

  if(entry && entry->latest && entry->latest != entry->current)
    setTarget(index, entry->latest, toggle);
}

void Browser::reinstall(const int index, const bool toggle)
{
  const Entry *entry = getEntry(index);

  if(entry && entry->current)
    setTarget(index, entry->current, toggle);
}

void Browser::installVersion(const int index, const size_t verIndex)
{
  const Entry *entry = getEntry(index);
  if(!entry)
    return;

  const auto &versions = entry->package->versions();

  if(verIndex >= versions.size())
    return;

  const Version *target = entry->package->version(verIndex);

  if(target == entry->current)
    resetTarget(index);
  else
    setTarget(index, target);
}

void Browser::uninstall(const int index, const bool toggle)
{
  const Entry *entry = getEntry(index);

  if(entry && entry->test(Entry::InstalledFlag) && !entry->test(Entry::ProtectedFlag))
    setTarget(index, nullptr, toggle);
}

void Browser::togglePin(const int index)
{
  Entry *entry = getEntry(index);
  if(!entry)
    return;

  const bool newVal = !entry->pin.value_or(entry->regEntry.pinned);

  if(newVal == entry->regEntry.pinned)
    entry->pin = boost::none;
  else
    entry->pin = newVal;

  updateAction(index);
}

bool Browser::hasAction(const Entry *entry) const
{
  return count(m_actions.begin(), m_actions.end(), entry) > 0;
}

void Browser::setTarget(const int index, const Version *target, const bool toggle)
{
  Entry *entry = getEntry(index);

  if(toggle && entry->target && *entry->target == target)
    entry->target = boost::none;
  else
    entry->target = target;

  updateAction(index);
}

void Browser::resetTarget(const int index)
{
  Entry *entry = getEntry(index);

  if(entry->target) {
    entry->target = boost::none;
    updateAction(index);
  }
}

void Browser::resetActions(const int index)
{
  Entry *entry = getEntry(index);

  if(entry->target)
    entry->target = boost::none;
  if(entry->pin)
    entry->pin = boost::none;

  updateAction(index);
}

void Browser::updateAction(const int index)
{
  Entry *entry = getEntry(index);
  if(!entry)
    return;

  const auto &it = find(m_actions.begin(), m_actions.end(), entry);
  if(!entry->target && (!entry->pin || !entry->test(Entry::CanTogglePin))) {
    if(it != m_actions.end())
      m_actions.erase(it);
  }
  else if(it == m_actions.end())
    m_actions.push_back(entry);

  if(currentView() == QueuedView && !hasAction(entry))
    m_list->removeRow(index);
  else
    m_list->row(index)->setCell(0, entry->displayState());
}

void Browser::listDo(const function<void (int)> &func, const vector<int> &indexes)
{
  ListView::BeginEdit edit(m_list);

  int lastSize = m_list->rowCount();
  int offset = 0;

  // Assumes the index vector is sorted
  for(const int index : indexes) {
    func(index - offset);

    // handle row removal
    int newSize = m_list->rowCount();
    if(newSize < lastSize)
      offset++;
    lastSize = newSize;
  }

  if(offset) // rows were deleted
    updateDisplayLabel();

  if(m_actions.empty())
    disable(m_applyBtn);
  else
    enable(m_applyBtn);
}

void Browser::currentDo(const function<void (int)> &func)
{
  listDo(func, {m_currentIndex});
}

void Browser::selectionDo(const function<void (int)> &func)
{
  listDo(func, m_list->selection());
}

auto Browser::currentView() const -> View
{
  return (View)SendMessage(m_view, CB_GETCURSEL, 0, 0);
}

void Browser::copy()
{
  vector<string> values;

  for(const int index : m_list->selection(false))
    values.push_back(getEntry(index)->displayName());

  setClipboard(values);
}

bool Browser::confirm() const
{
  // count uninstallation actions
  const size_t count = count_if(m_actions.begin(), m_actions.end(),
    [](const Entry *e) { return e->target && *e->target == nullptr; });

  if(!count)
    return true;

  char msg[255];
  snprintf(msg, sizeof(msg),
    "Are you sure to uninstall %zu package%s?\nThe files and settings will"
    " be permanently deleted from this computer.",
    count, count == 1 ? "" : "s");

  return IDYES == Win32::messageBox(handle(), msg, "ReaPack Query", MB_YESNO);
}

bool Browser::apply()
{
  if(m_actions.empty())
    return true;

  Transaction *tx = g_reapack->setupTransaction();

  if(!tx)
    return false;

  for(Entry *entry : m_actions) {
    if(entry->target) {
      const Version *target = *entry->target;

      if(target)
        tx->install(target, entry->pin.value_or(false));
      else
        tx->uninstall(entry->regEntry);

      entry->target = boost::none;
    }
    else if(entry->pin) {
      tx->setPinned(entry->regEntry, *entry->pin);
      entry->pin = boost::none;
    }
  }

  m_actions.clear();
  disable(m_applyBtn);

  if(!tx->runTasks()) {
    // This is an asynchronous transaction.
    // Updating the state column of all rows (not just visible ones since the
    // hidden rows can be filtered into view again by user at any time) right away
    // to give visual feedback.
    ListView::BeginEdit edit(m_list);

    if(currentView() == QueuedView)
      m_list->clear();
    else {
      for(int i = 0, count = m_list->rowCount(); i < count; ++i)
        m_list->row(i)->setCell(0, getEntry(i)->displayState());
    }
  }

  return true;
}
