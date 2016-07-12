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

#include "browser.hpp"

#include "config.hpp"
#include "encoding.hpp"
#include "errors.hpp"
#include "index.hpp"
#include "menu.hpp"
#include "reapack.hpp"
#include "report.hpp"
#include "resource.hpp"
#include "transaction.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <locale>
#include <sstream>

using namespace std;

enum Action {
  ACTION_VERSION = 80,
  ACTION_FILTERTYPE,
  ACTION_LATEST = 300,
  ACTION_LATEST_ALL,
  ACTION_REINSTALL,
  ACTION_REINSTALL_ALL,
  ACTION_UNINSTALL,
  ACTION_UNINSTALL_ALL,
  ACTION_PIN,
  ACTION_ABOUT_PKG,
  ACTION_ABOUT_REMOTE,
  ACTION_RESET_ALL,
  ACTION_SHOWDESCS,
  ACTION_REFRESH,
  ACTION_MANAGE,
};

Browser::Browser(ReaPack *reapack)
  : Dialog(IDD_BROWSER_DIALOG), m_reapack(reapack),
    m_loading(false), m_loaded(false), m_checkFilter(false), m_currentIndex(-1)
{
}

void Browser::onInit()
{
  m_applyBtn = getControl(IDAPPLY);
  m_filterHandle = getControl(IDC_FILTER);
  m_view = getControl(IDC_TABS);
  m_displayBtn = getControl(IDC_DISPLAY);
  m_actionsBtn = getControl(IDC_ACTIONS);

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
    {AUTO_STR("Package"), 380},
    {AUTO_STR("Category"), 150},
    {AUTO_STR("Version"), 80},
    {AUTO_STR("Author"), 95},
    {AUTO_STR("Type"), 70},
    {AUTO_STR("Repository"), 120, ListView::CollapseFlag},
    {AUTO_STR("Last Update"), 120, ListView::CollapseFlag},
  });

  m_list->onActivate([=] { aboutPackage(m_list->itemUnderMouse()); });
  m_list->onSelect([=] { setEnabled(m_list->hasSelection(), m_actionsBtn); });
  m_list->onContextMenu(bind(&Browser::fillContextMenu,
    this, placeholders::_1, placeholders::_2));

  m_list->sortByColumn(1);
  m_list->setSortCallback(7 /* last update */, [&] (const int ai, const int bi) {
    const Entry &a = m_entries[ai];
    const Entry &b = m_entries[bi];

    if(!a.latest)
      return -1;
    else if(!b.latest)
      return 1;

    return a.latest->time().compare(b.latest->time());
  });

  m_filterTimer = startTimer(200);

  Dialog::onInit();
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

  auto data = m_serializer.read(m_reapack->config()->browser()->state, 1);
  Dialog::restore(data);
  m_list->restore(data);
  assert(data.empty());

  updateDisplayLabel();
  refresh();
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
    m_checkFilter = true;
    break;
  case IDC_CLEAR:
    SetWindowText(m_filterHandle, AUTO_STR(""));
    checkFilter();
    SetFocus(m_filterHandle);
    break;
  case IDC_SELECT:
    m_list->selectAll();
    SetFocus(m_list->handle());
    break;
  case IDC_UNSELECT:
    m_list->unselectAll();
    SetFocus(m_list->handle());
    break;
  case IDC_ACTIONS:
    actionsButton();
    break;
  case ACTION_LATEST:
    installLatest(m_currentIndex);
    break;
  case ACTION_LATEST_ALL:
    selectionDo(bind(&Browser::installLatest, this, placeholders::_1, false));
    break;
  case ACTION_REINSTALL:
    reinstall(m_currentIndex);
    break;
  case ACTION_REINSTALL_ALL:
    selectionDo(bind(&Browser::reinstall, this, placeholders::_1, false));
    break;
  case ACTION_UNINSTALL:
    uninstall(m_currentIndex);
    break;
  case ACTION_UNINSTALL_ALL:
    selectionDo(bind(&Browser::uninstall, this, placeholders::_1, false));
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
    selectionDo(bind(&Browser::resetActions, this, placeholders::_1));
    break;
  case ACTION_SHOWDESCS:
    toggleDescs();
    break;
  case ACTION_REFRESH:
    refresh(true);
    break;
  case ACTION_MANAGE:
    m_reapack->manageRemotes();
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
    if(m_loading)
      hide();
    else
      close();
    break;
  default:
    if(id >> 8 == ACTION_VERSION)
      installVersion(m_currentIndex, id & 0xff);
    else if(id >> 8 == ACTION_FILTERTYPE)
      toggleFiltered(static_cast<Package::Type>(id & 0xff));
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
      values.push_back(getValue(NameColumn, *getEntry(index)));

    setClipboard(values);
  }
  else if(!mods && key == VK_F5)
    refresh(true);
  else
    return false;

  return true;
}

void Browser::onClose()
{
  Serializer::Data data;
  Dialog::save(data);
  m_list->save(data);
  m_reapack->config()->browser()->state = m_serializer.write(data);
}

void Browser::onTimer(const int id)
{
  if(id == m_filterTimer)
    checkFilter();
}

bool Browser::fillContextMenu(Menu &menu, const int index)
{
  m_currentIndex = index;
  fillMenu(menu);

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
  }

  if(!entry) {
    menu.addAction(AUTO_STR("&Select all"), IDC_SELECT);
    menu.addAction(AUTO_STR("&Unselect all"), IDC_UNSELECT);
    return;
  }

  if(entry->test(InstalledFlag)) {
    if(entry->test(OutOfDateFlag)) {
      auto_char installLabel[32] = {};
      auto_snprintf(installLabel, auto_size(installLabel),
        AUTO_STR("U&pdate to v%s"),
        make_autostring(entry->latest->name()).c_str());

      const UINT actionIndex = menu.addAction(installLabel, ACTION_LATEST);
      if(entry->target && *entry->target == entry->latest)
        menu.check(actionIndex);
    }

    auto_char reinstallLabel[32] = {};
    auto_snprintf(reinstallLabel, auto_size(reinstallLabel),
      AUTO_STR("&Reinstall v%s"),
      make_autostring(entry->regEntry.version.name()).c_str());

    const UINT actionIndex = menu.addAction(reinstallLabel, ACTION_REINSTALL);
    if(!entry->current || entry->test(ObsoleteFlag))
      menu.disable(actionIndex);
    else if(entry->target && *entry->target == entry->current)
      menu.check(actionIndex);
  }
  else {
    auto_char installLabel[32] = {};
    auto_snprintf(installLabel, auto_size(installLabel),
      AUTO_STR("&Install v%s"),
      make_autostring(entry->latest->name()).c_str());

    const UINT actionIndex = menu.addAction(installLabel, ACTION_LATEST);
    if(entry->target && *entry->target == entry->latest)
      menu.check(actionIndex);
  }

  Menu versionMenu = menu.addMenu(AUTO_STR("Versions"));
  const UINT versionMenuIndex = menu.size() - 1;
  if(entry->test(ObsoleteFlag))
    menu.disable(versionMenuIndex);
  else {
    const auto &versions = entry->package->versions();
    int verIndex = (int)versions.size();
    for(const Version *ver : versions | boost::adaptors::reversed) {
      const UINT actionIndex = versionMenu.addAction(
        make_autostring(ver->name()).c_str(), --verIndex | (ACTION_VERSION << 8));

      if(entry->target ? *entry->target == ver : ver == entry->current) {
        if(entry->target && ver != entry->latest)
          menu.check(versionMenuIndex);

        versionMenu.checkRadio(actionIndex);
      }
    }
  }

  const UINT pinIndex = menu.addAction(
    AUTO_STR("&Pin current version"), ACTION_PIN);
  if(!entry->canPin())
    menu.disable(pinIndex);
  if(entry->pin.value_or(entry->regEntry.pinned))
    menu.check(pinIndex);

  const UINT uninstallIndex =
    menu.addAction(AUTO_STR("&Uninstall"), ACTION_UNINSTALL);
  if(!entry->test(InstalledFlag) || getRemote(*entry).isProtected())
    menu.disable(uninstallIndex);
  else if(entry->target && *entry->target == nullptr)
    menu.check(uninstallIndex);

  menu.addSeparator();

  menu.setEnabled(!entry->test(ObsoleteFlag),
    menu.addAction(AUTO_STR("About this &package"), ACTION_ABOUT_PKG));

  auto_char aboutLabel[32] = {};
  const auto_string &name = make_autostring(getValue(RemoteColumn, *entry));
  auto_snprintf(aboutLabel, auto_size(aboutLabel),
    AUTO_STR("&About %s..."), name.c_str());
  menu.addAction(aboutLabel, ACTION_ABOUT_REMOTE);
}

void Browser::updateDisplayLabel()
{
  basic_ostringstream<auto_char> btnLabel;
  btnLabel.imbue(locale("")); // enable number formatting
  btnLabel << m_list->rowCount() << AUTO_STR('/') << m_entries.size()
    << AUTO_STR(" package");

  if(m_entries.size() != 1)
    btnLabel << AUTO_STR('s');

  SetWindowText(m_displayBtn, btnLabel.str().c_str());
}

void Browser::displayButton()
{
  RECT rect;
  GetWindowRect(m_displayBtn, &rect);

  static map<const auto_char *, Package::Type> types = {
    {AUTO_STR("&Scripts"), Package::ScriptType},
    {AUTO_STR("&Effects"), Package::EffectType},
    {AUTO_STR("E&xtensions"), Package::ExtensionType},
    {AUTO_STR("&Themes"), Package::ThemeType},
    {AUTO_STR("&Other packages"), Package::UnknownType},
  };

  Menu menu;
  for(const auto &pair : types) {
    const auto index = menu.addAction(pair.first,
      pair.second | (ACTION_FILTERTYPE << 8));

    if(!isFiltered(pair.second))
      menu.check(index);
  }

  const auto config = m_reapack->config()->browser();
  menu.addSeparator();

  const auto i = menu.addAction(AUTO_STR("Show &descriptions"), ACTION_SHOWDESCS);
  if(config->showDescs)
    menu.check(i);

  menu.addAction(AUTO_STR("&Refresh repositories"), ACTION_REFRESH);
  menu.addAction(AUTO_STR("&Manage repositories..."), ACTION_MANAGE);

  menu.show(rect.left, rect.bottom - 1, handle());
}

void Browser::actionsButton()
{
  RECT rect;
  GetWindowRect(m_actionsBtn, &rect);

  m_currentIndex = m_list->currentIndex();

  Menu menu;
  fillMenu(menu);
  menu.show(rect.left, rect.bottom - 1, handle());
}

bool Browser::isFiltered(Package::Type type) const
{
  switch(type) {
  case Package::ScriptType:
  case Package::EffectType:
  case Package::ExtensionType:
  case Package::ThemeType:
    break;
  default:
    type = Package::UnknownType;
    break;
  }

  const auto config = m_reapack->config()->browser();
  return ((config->typeFilter >> type) & 1) == 1;
}

void Browser::toggleFiltered(const Package::Type type)
{
  auto config = m_reapack->config()->browser();
  config->typeFilter ^= 1 << type;
  fillList();
}

void Browser::toggleDescs()
{
  auto config = m_reapack->config()->browser();
  config->showDescs = !config->showDescs;
  fillList();
}

void Browser::checkFilter()
{
  if(!m_checkFilter)
    return;

  m_checkFilter = false;

  auto_string wideFilter(4096, 0);
  GetWindowText(m_filterHandle, &wideFilter[0], (int)wideFilter.size());
  wideFilter.resize(wideFilter.find(AUTO_STR('\0')));

  const string &filter = from_autostring(wideFilter);

  if(m_filter != filter) {
    m_filter = filter;
    fillList();
  }
}

void Browser::refresh(const bool stale)
{
  if(m_loading)
    return;

  const vector<Remote> &remotes = m_reapack->config()->remotes()->getEnabled();

  if(!m_loaded && remotes.empty()) {
    m_loaded = true;
    show();

    MessageBox(handle(), AUTO_STR("No repository enabled!\r\n")
      AUTO_STR("Enable or import repositories from ")
      AUTO_STR("Extensions > ReaPack > Manage repositories."),
      AUTO_STR("Browse packages"), MB_OK);

    return;
  }

  m_loading = true;

  m_reapack->fetchIndexes(remotes, [=] (vector<IndexPtr> indexes) {
    // if the user wanted to close the window while we were downloading stuff
    if(m_loaded && !isVisible()) {
      close();
      return;
    }

    m_loading = false;

    // keep the old indexes around a little bit longer for use by #populate
    indexes.swap(m_indexes);

    populate();

    if(!m_loaded) {
      m_loaded = true;
      show();
    }
  }, handle(), stale);
}

void Browser::populate()
{
  try {
    Registry reg(Path::prefixRoot(Path::REGISTRY));

    // keep previous entries in memory a bit longer for #transferActions
    vector<Entry> oldEntries;
    swap(m_entries, oldEntries);

    m_currentIndex = -1;

    // prevent #fillList from trying to restore the selection
    // (entry indexes may mismatch depending on the new repository contents,
    // thus causing the wrong package to be selected)
    m_visibleEntries.clear();

    for(IndexPtr index : m_indexes) {
      for(const Package *pkg : index->packages())
        m_entries.push_back(makeEntry(pkg, reg.getEntry(pkg)));

      // obsolete packages
      for(const Registry::Entry &regEntry : reg.getEntries(index->name())) {
        const Category *cat = index->category(regEntry.category);

        if(cat && cat->package(regEntry.package))
          continue;

        m_entries.push_back({InstalledFlag | ObsoleteFlag, regEntry});
      }
    }

    transferActions();
    fillList();
  }
  catch(const reapack_error &e) {
    const auto_string &desc = make_autostring(e.what());
    auto_char msg[255] = {};
    auto_snprintf(msg, auto_size(msg),
      AUTO_STR("ReaPack could not read from its package registry.\r\n")
      AUTO_STR("Retry later once all installation task are completed.\r\n")
      AUTO_STR("\r\nError description: %s"),
      desc.c_str());
    MessageBox(handle(), msg, AUTO_STR("ReaPack"), MB_OK);
  }
}

void Browser::transferActions()
{
  unordered_set<Entry *> oldActions;
  swap(m_actions, oldActions);

  for(Entry *oldEntry : oldActions) {
    const auto &entryIt = find(m_entries.begin(), m_entries.end(), *oldEntry);
    if(entryIt == m_entries.end())
      continue;

    if(oldEntry->target) {
      const Version *target = *oldEntry->target;

      if(target) {
        const Package *pkg = entryIt->package;
        if(!pkg || !(target = pkg->findVersion(*target)))
          continue;
      }

      entryIt->target = target;
    }

    if(oldEntry->pin)
      entryIt->pin = *oldEntry->pin;

    m_actions.insert(&*entryIt);
  }

  if(m_actions.empty())
    disable(m_applyBtn);
}

auto Browser::makeEntry(const Package *pkg, const Registry::Entry &regEntry)
  const -> Entry
{
  const auto &instOpts = *m_reapack->config()->install();
  const Version *latest = pkg->lastVersion(instOpts.bleedingEdge, regEntry.version);
  const Version *current = nullptr;

  int flags = 0;

  if(regEntry) {
    flags |= InstalledFlag;

    if(latest && regEntry.version < *latest)
      flags |= OutOfDateFlag;

    current = pkg->findVersion(regEntry.version);
  }
  else {
    if(!latest) // show latest pre-release if no stable version is available
      latest = pkg->lastVersion(true);

    flags |= UninstalledFlag;
  }

  return {flags, regEntry, pkg, latest, current};
}

void Browser::fillList()
{
  InhibitControl freeze(m_list);

  // store the indexes to the selected entries if they still exists
  // and m_visibleEntries hasn't been emptied
  const vector<int> selection = m_list->selection();
  vector<size_t> selected(min(selection.size(), m_visibleEntries.size()));
  for(size_t i = 0; i < selected.size(); i++)
    selected[i] = m_visibleEntries[selection[i]];

  m_list->clear();
  m_visibleEntries.clear();

  for(size_t i = 0; i < m_entries.size(); i++) {
    const Entry &entry = m_entries[i];

    if(!match(entry))
      continue;

    const int index = m_list->addRow(makeRow(entry));

    if(find(selected.begin(), selected.end(), i) != selected.end())
      m_list->select(index);

    m_visibleEntries.push_back(i);
  }

  m_list->sort();

  updateDisplayLabel();
}

ListView::Row Browser::makeRow(const Entry &entry) const
{
  const string &state = getValue(StateColumn, entry);
  const string &name = getValue(NameColumn, entry);
  const string &category = getValue(CategoryColumn, entry);
  const string &version = getValue(VersionColumn, entry);
  const string &author = getValue(AuthorColumn, entry);
  const string &type = getValue(TypeColumn, entry);
  const string &remote = getValue(RemoteColumn, entry);
  const string &date = getValue(TimeColumn, entry);

  return {
    make_autostring(state), make_autostring(name), make_autostring(category),
    make_autostring(version), make_autostring(author), make_autostring(type),
    make_autostring(remote), make_autostring(date),
  };
}

string Browser::getValue(const Column col, const Entry &entry) const
{
  const Package *pkg = entry.package;
  const Version *ver = entry.latest;
  const Registry::Entry &regEntry = entry.regEntry;

  string display;

  switch(col) {
  case StateColumn: {
    if(entry.test(ObsoleteFlag))
      display += 'o';
    else if(entry.regEntry.pinned)
      display += 'p';
    else if(entry.test(OutOfDateFlag))
      display += 'u';
    else if(entry.test(InstalledFlag))
      display += 'i';
    else
      display += '\x20';

    if(entry.target)
      display += *entry.target == nullptr ? 'R' : 'I';
    if(entry.pin && entry.canPin())
      display += 'P';

    return display;
  }
  case NameColumn: {
    const auto config = m_reapack->config()->browser();

    if(pkg) {
      if(!config->showDescs || pkg->description().empty())
        return pkg->name();
      else
        return pkg->description();
    }
    else
      return regEntry.package;
  }
  case CategoryColumn:
    return pkg ? pkg->category()->name() : regEntry.category;
  case VersionColumn:
    if(entry.test(InstalledFlag))
      display = regEntry.version.name();

    if(ver && (!regEntry || *ver > regEntry.version)) {
      if(!display.empty())
        display += '\x20';

      display += '(' + ver->name() + ')';
    }

    return display;
  case AuthorColumn:
    return ver ? ver->displayAuthor() : regEntry.version.displayAuthor();
  case TypeColumn:
    return pkg ? pkg->displayType() : Package::displayType(regEntry.type);
  case RemoteColumn:
    return pkg ? pkg->category()->index()->name() : regEntry.remote;
  case TimeColumn:
    return ver ? ver->time().toString() : string();
  }

  return {}; // for MSVC
}

Remote Browser::getRemote(const Entry &entry) const
{
  return m_reapack->remote(getValue(RemoteColumn, entry));
}

bool Browser::match(const Entry &entry) const
{
  switch(currentView()) {
  case AllView:
    break;
  case QueuedView:
    if(!m_actions.count(const_cast<Entry *>(&entry)))
      return false;
    break;
  case InstalledView:
    if(!entry.test(InstalledFlag))
      return false;
    break;
  case OutOfDateView:
    if(!entry.test(OutOfDateFlag))
      return false;
    break;
  case UninstalledView:
    if(!entry.test(UninstalledFlag))
      return false;
    break;
  case ObsoleteView:
    if(!entry.test(ObsoleteFlag))
      return false;
    break;
  }

  const Package::Type type =
    entry.latest ? entry.package->type() : entry.regEntry.type;

  if(isFiltered(type))
    return false;

  const string &name = getValue(NameColumn, entry);
  const string &category = getValue(CategoryColumn, entry);
  const string &author = getValue(AuthorColumn, entry);
  const string &remote = getValue(RemoteColumn, entry);

  return m_filter.match(name + category + author + remote);
}

auto Browser::getEntry(const int listIndex) -> Entry *
{
  if(listIndex < 0 || listIndex >= (int)m_visibleEntries.size())
    return nullptr;

  return &m_entries[m_visibleEntries[listIndex]];
}

void Browser::aboutPackage(const int index)
{
  if(const Entry *entry = getEntry(index))
    m_reapack->about(entry->package, handle());
}

void Browser::aboutRemote(const int index)
{
  if(const Entry *entry = getEntry(index))
    m_reapack->about(getRemote(*entry), handle());
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

  if(entry && entry->test(InstalledFlag) && !getRemote(*entry).isProtected())
    setTarget(index, nullptr, toggle);
}

void Browser::togglePin(const int index)
{
  Entry *entry = getEntry(index);
  if(!entry)
    return;

  const bool newVal = !entry->pin.value_or(entry->regEntry.pinned);

  if(newVal == entry->regEntry.pinned) {
    entry->pin = boost::none;
    if(!entry->target)
      m_actions.erase(entry);
  }
  else {
    entry->pin = newVal;
    m_actions.insert(entry);
  }

  updateAction(index);
}

void Browser::setTarget(const int index, const Version *target, const bool toggle)
{
  Entry *entry = getEntry(index);

  if(toggle && entry->target && *entry->target == target)
    resetTarget(index);
  else {
    entry->target = target;
    m_actions.insert(entry);
    updateAction(index);
  }
}

void Browser::resetTarget(const int index)
{
  Entry *entry = getEntry(index);

  if(!entry->target)
    return;

  entry->target = boost::none;
  if(!entry->pin || !entry->canPin())
    m_actions.erase(entry);

  updateAction(index);
}

void Browser::resetActions(const int index)
{
  Entry *entry = getEntry(index);
  if(!m_actions.count(entry))
    return;

  if(entry->target)
    entry->target = boost::none;
  if(entry->pin)
    entry->pin = boost::none;

  m_actions.erase(entry);
  updateAction(index);
}

void Browser::updateAction(const int index)
{
  Entry *entry = getEntry(index);
  if(!entry)
    return;

  if(currentView() == QueuedView && !m_actions.count(entry)) {
    m_list->removeRow(index);
    m_visibleEntries.erase(m_visibleEntries.begin() + index);
    updateDisplayLabel();
  }
  else
    m_list->replaceRow(index, makeRow(*entry));

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
  if(m_actions.empty())
    return true;

  const size_t count = m_actions.size();

  auto_char msg[255] = {};
  auto_snprintf(msg, auto_size(msg),
    AUTO_STR("Confirm execution of %zu action%s?\n"),
    count, count == 1 ? AUTO_STR("") : AUTO_STR("s"));

  const auto_char *title = AUTO_STR("ReaPack Query");
  const int btn = MessageBox(handle(), msg, title, MB_YESNO);

  return btn == IDYES;
}

bool Browser::apply()
{
  if(m_actions.empty())
    return true;

  Transaction *tx = m_reapack->setupTransaction();

  if(!tx)
    return false;

  for(Entry *entry : m_actions) {
    if(entry->target) {
      const Version *target = *entry->target;

      if(target)
        tx->install(target);
      else
        tx->uninstall(entry->regEntry);

      entry->target = boost::none;
    }

    if(entry->pin) {
      if(entry->regEntry)
        tx->setPinned(entry->regEntry, *entry->pin);
      else
        tx->setPinned(entry->package, *entry->pin);

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

auto Browser::Entry::hash() const -> Hash
{
  if(package) {
    return make_tuple(package->category()->index()->name(),
      package->category()->name(), package->name());
  }
  else
    return make_tuple(regEntry.remote, regEntry.category, regEntry.package);
}
