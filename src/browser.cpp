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
#include "encoding.hpp"
#include "errors.hpp"
#include "index.hpp"
#include "listview.hpp"
#include "menu.hpp"
#include "reapack.hpp"
#include "resource.hpp"
#include "transaction.hpp"

using namespace std;

enum Timers { TIMER_FILTER = 1, TIMER_ABOUT };

Browser::Browser()
  : Dialog(IDD_BROWSER_DIALOG), m_loadState(Init), m_currentIndex(-1)
{
}

void Browser::onInit()
{
  m_applyBtn = getControl(IDAPPLY);
  m_filterHandle = getControl(IDC_FILTER);
  m_view = getControl(IDC_TABS);
  m_displayBtn = getControl(IDC_DISPLAY);
  m_actionsBtn = getControl(IDC_ACTION);

  disable(m_applyBtn);
  disable(m_actionsBtn);

  // don't forget to update order of enum View in header file
  SendMessage(m_view, CB_ADDSTRING, 0, (LPARAM)AUTO_STR("All"));
  SendMessage(m_view, CB_ADDSTRING, 0, (LPARAM)AUTO_STR("Queued"));
  SendMessage(m_view, CB_ADDSTRING, 0, (LPARAM)AUTO_STR("Installed"));
  SendMessage(m_view, CB_ADDSTRING, 0, (LPARAM)AUTO_STR("Out of date"));
  SendMessage(m_view, CB_ADDSTRING, 0, (LPARAM)AUTO_STR("Obsolete"));
  SendMessage(m_view, CB_ADDSTRING, 0, (LPARAM)AUTO_STR("Uninstalled"));
  SendMessage(m_view, CB_SETCURSEL, 0, 0);

  m_list = createControl<ListView>(IDC_LIST, ListView::Columns{
    {AUTO_STR("Status"), 23, ListView::NoLabelFlag},
    {AUTO_STR("Package"), 345},
    {AUTO_STR("Category"), 105},
    {AUTO_STR("Version"), 55, 0, ListView::VersionType},
    {AUTO_STR("Author"), 95},
    {AUTO_STR("Type"), 70},
    {AUTO_STR("Repository"), 120, ListView::CollapseFlag},
    {AUTO_STR("Last Update"), 105, 0, ListView::TimeType},
  });

  m_list->onActivate([=] { aboutPackage(m_list->itemUnderMouse()); });
  m_list->onSelect(bind(&Browser::onSelection, this));
  m_list->onContextMenu(bind(&Browser::fillContextMenu, this, _1, _2));
  m_list->sortByColumn(1);

  Dialog::onInit();
  setMinimumSize({600, 250});

  setAnchor(m_filterHandle, AnchorRight);
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
    installLatest(m_currentIndex);
    break;
  case ACTION_LATEST_ALL:
    installLatestAll();
    break;
  case ACTION_REINSTALL:
    reinstall(m_currentIndex);
    break;
  case ACTION_REINSTALL_ALL:
    selectionDo(bind(&Browser::reinstall, this, _1, false));
    break;
  case ACTION_UNINSTALL:
    uninstall(m_currentIndex);
    break;
  case ACTION_UNINSTALL_ALL:
    selectionDo(bind(&Browser::uninstall, this, _1, false));
    break;
  case ACTION_PIN:
    togglePin(m_currentIndex);
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
    if(m_loadState >= Loading)
      hide();
    else
      close();
    break;
  default:
    if(id >> 8 == ACTION_VERSION)
      installVersion(m_currentIndex, id & 0xff);
    else if(id >> 8 == ACTION_FILTERTYPE) {
      m_typeFilter = static_cast<Package::Type>(id & 0xff);
      fillList();
    }
    break;
  }
}

bool Browser::onKeyDown(const int key, const int mods)
{
  if(GetFocus() != m_list->handle())
    return false;

  if(mods == CtrlModifier && key == 'A')
    m_list->selectAll();
  else if(mods == (CtrlModifier | ShiftModifier) && key == 'A')
    m_list->unselectAll();
  else if(mods == CtrlModifier && key == 'C') {
    vector<string> values;

    for(const int index : m_list->selection(false))
      values.push_back(getEntry(index)->displayName());

    setClipboard(values);
  }
  else if(!mods && key == VK_F5)
    refresh(true);
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

  if(!menu.empty())
    menu.addSeparator();

  menu.addAction(AUTO_STR("&Select all"), IDC_SELECT);
  menu.addAction(AUTO_STR("&Unselect all"), IDC_UNSELECT);

  return true;
}

void Browser::fillMenu(Menu &menu)
{
  const Entry *entry = getEntry(m_currentIndex);

  if(m_list->selectionSize() > 1) {
    menu.addAction(AUTO_STR("&Install/update selection"), ACTION_LATEST_ALL);
    menu.addAction(AUTO_STR("&Reinstall selection"), ACTION_REINSTALL_ALL);
    menu.addAction(AUTO_STR("&Uninstall selection"), ACTION_UNINSTALL_ALL);
    menu.addAction(AUTO_STR("&Clear queued actions"), ACTION_RESET_ALL);
    menu.addSeparator();

    if(entry) {
      Menu pkgMenu = menu.addMenu(AUTO_STR("Package under cursor"));
      entry->fillMenu(pkgMenu);
    }
  }
  else if(entry)
    entry->fillMenu(menu);
}

void Browser::updateDisplayLabel()
{
  auto_char btnLabel[32];
  auto_snprintf(btnLabel, auto_size(btnLabel), AUTO_STR("%d/%zu package%s..."),
    m_list->rowCount(), m_entries.size(),
    m_entries.size() == 1 ? AUTO_STR("") : AUTO_STR("s"));

  SetWindowText(m_displayBtn, btnLabel);
}

void Browser::displayButton()
{
  static map<const auto_char *, Package::Type> types = {
    {AUTO_STR("&Scripts"), Package::ScriptType},
    {AUTO_STR("&Effects"), Package::EffectType},
    {AUTO_STR("E&xtensions"), Package::ExtensionType},
    {AUTO_STR("&Themes"), Package::ThemeType},
    {AUTO_STR("&Language Packs"), Package::LangPackType},
    {AUTO_STR("&Web Interfaces"), Package::WebInterfaceType},
    {AUTO_STR("&Project Templates"), Package::ProjectTemplateType},
    {AUTO_STR("&Track Templates"), Package::TrackTemplateType},
    {AUTO_STR("&MIDI Note Names"), Package::MIDINoteNamesType},
    {AUTO_STR("&Other packages"), Package::UnknownType},
  };

  Menu menu;

  auto index = menu.addAction(AUTO_STR("&All packages"), ACTION_FILTERTYPE);
  if(!m_typeFilter)
    menu.checkRadio(index);

  for(const auto &pair : types) {
    auto index = menu.addAction(pair.first,
      pair.second | (ACTION_FILTERTYPE << 8));

    if(m_typeFilter && m_typeFilter == pair.second)
      menu.checkRadio(index);
  }

  menu.addSeparator();

  menu.addAction(AUTO_STR("&Refresh repositories"), ACTION_REFRESH);
  menu.addAction(AUTO_STR("&Manage repositories..."), ACTION_MANAGE);

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

  const string &filter = getText(m_filterHandle);

  if(m_filter != filter) {
    m_filter = filter;
    fillList();
  }
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
  // Do nothing when called again when (or while) the index downloading
  // transaction finishes. populate() handles the next step of the loading process.
  switch(m_loadState) {
  case Done:
    m_loadState = Loaded;
  case Loading:
    return;
  default:
    break;
  }

  const vector<Remote> &remotes = g_reapack->config()->remotes.getEnabled();

  if(remotes.empty()) {
    if(!isVisible() || stale) {
      show();

      MessageBox(handle(), AUTO_STR("No repository enabled!\r\n")
        AUTO_STR("Enable or import repositories from ")
        AUTO_STR("Extensions > ReaPack > Manage repositories."),
        AUTO_STR("Browse packages"), MB_OK);
    }

    populate({});
    return;
  }

  if(Transaction *tx = g_reapack->setupTransaction()) {
    const bool firstLoad = m_loadState == Init;
    m_loadState = Loading;

    tx->fetchIndexes(remotes, stale);
    tx->onFinish([=] {
      m_loadState = Done;

      if(firstLoad || isVisible())
        populate(tx->getIndexes(remotes));
      else
        close();
    });

    tx->runTasks();
  }
}

void Browser::setFilter(const string &newFilter)
{
  SetWindowText(m_filterHandle, make_autostring(newFilter).c_str());
  updateFilter(); // don't wait for the timer, update now!
  SetFocus(m_filterHandle);
}

void Browser::populate(const vector<IndexPtr> &indexes)
{
  try {
    Registry reg(Path::prefixRoot(Path::REGISTRY));

    // keep previous entries in memory a bit longer for #transferActions
    vector<Entry> oldEntries;
    swap(m_entries, oldEntries);

    m_currentIndex = -1;

    for(const IndexPtr &index : indexes) {
      for(const Package *pkg : index->packages())
        m_entries.push_back({pkg, reg.getEntry(pkg), index});

      // obsolete packages
      for(const Registry::Entry &regEntry : reg.getEntries(index->name())) {
        if(!index->find(regEntry.category, regEntry.package))
          m_entries.push_back({regEntry, index});
      }
    }

    transferActions();
    fillList();
  }
  catch(const reapack_error &e) {
    const auto_string &desc = make_autostring(e.what());
    auto_char msg[255];
    auto_snprintf(msg, auto_size(msg),
      AUTO_STR("ReaPack could not read from the local package registry.\r\n")
      AUTO_STR("Retry later once all installation task are completed.\r\n")
      AUTO_STR("\r\nError description: %s"),
      desc.c_str());
    MessageBox(handle(), msg, AUTO_STR("ReaPack"), MB_OK);
  }

  if(!isVisible())
    show();
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
  InhibitControl freeze(m_list);

  const int scroll = m_list->scroll();

  const vector<int> selectedIndexes = m_list->selection();
  vector<const Entry *> oldSelection(selectedIndexes.size());
  for(size_t i = 0; i < selectedIndexes.size(); i++)
    oldSelection[i] = (Entry *)m_list->row(selectedIndexes[i]).userData;

  m_list->clear();
  m_list->reserveRows(m_entries.size());

  for(const Entry &entry : m_entries) {
    if(!match(entry))
      continue;

    const auto &matchingEntryIt = find_if(oldSelection.begin(), oldSelection.end(),
      [&entry] (const Entry *oldEntry) { return *oldEntry == entry; });

    const int index = m_list->addRow(entry.makeRow());

    if(matchingEntryIt != oldSelection.end())
      m_list->select(index);
  }

  m_list->setScroll(scroll);
  m_list->sort();

  updateDisplayLabel();
}

bool Browser::match(const Entry &entry) const
{
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

  if(isFiltered(entry.type()))
    return false;

  return m_filter.match({entry.displayName(), entry.categoryName(),
    entry.displayAuthor(), entry.indexName()});
}

auto Browser::getEntry(const int index) -> Entry *
{
  if(index < 0)
    return nullptr;
  else
    return (Entry *)m_list->row(index).userData;
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
    const int btn = MessageBox(handle(),
      AUTO_STR("Do you want ReaPack to install new packages automatically when")
      AUTO_STR(" synchronizing in the future?\r\n\r\nThis setting can also be")
      AUTO_STR(" customized globally or on a per-repository basis in")
      AUTO_STR(" ReaPack > Manage repositories."),
      AUTO_STR("Install every available packages"), MB_YESNOCANCEL);

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

  const auto versions = entry->package->versions();

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

  if(entry && entry->test(Entry::InstalledFlag) && !entry->remote().isProtected())
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

  const auto it = find(m_actions.begin(), m_actions.end(), entry);
  if(!entry->target && (!entry->pin || !entry->canPin())) {
    if(it != m_actions.end())
      m_actions.erase(it);
  }
  else if(it == m_actions.end())
    m_actions.push_back(entry);

  if(currentView() == QueuedView && !hasAction(entry)) {
    m_list->removeRow(index);
    updateDisplayLabel();
  }
  else {
    m_list->setCell(index, 0, entry->displayState());

    if(m_list->sortColumn() == 0)
      m_list->sort();
  }

  if(m_actions.empty())
    disable(m_applyBtn);
  else
    enable(m_applyBtn);
}

void Browser::selectionDo(const function<void (int)> &func)
{
  InhibitControl freeze(m_list);

  int lastSize = m_list->rowCount();
  int offset = 0;

  for(const int index : m_list->selection()) {
    func(index - offset);

    // handle row removal
    int newSize = m_list->rowCount();
    if(newSize < lastSize)
      offset++;
    lastSize = newSize;
  }
}

auto Browser::currentView() const -> View
{
  return (View)SendMessage(m_view, CB_GETCURSEL, 0, 0);
}

bool Browser::confirm() const
{
  // count uninstallation actions
  const size_t count = count_if(m_actions.begin(), m_actions.end(),
    [](const Entry *e) { return e->target && *e->target == nullptr; });

  if(!count)
    return true;

  auto_char msg[255];
  auto_snprintf(msg, auto_size(msg),
    AUTO_STR("Are you sure to uninstall %zu package%s?\r\nThe files and settings will be permanently deleted from this computer."),
    count, count == 1 ? AUTO_STR("") : AUTO_STR("s"));

  const auto_char *title = AUTO_STR("ReaPack Query");
  const int btn = MessageBox(handle(), msg, title, MB_YESNO);

  return btn == IDYES;
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
    // this is an asynchronous transaction
    // update the state column right away to give visual feedback
    fillList();
  }

  return true;
}
