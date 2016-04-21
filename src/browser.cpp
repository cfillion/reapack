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
  ACTION_CONTENTS,
  ACTION_HISTORY,
  ACTION_ABOUT,
  ACTION_RESET_ALL,
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
  m_apply = getControl(IDAPPLY);
  m_filterHandle = getControl(IDC_FILTER);
  m_tabs = getControl(IDC_TABS);
  m_display = getControl(IDC_DISPLAY);

  disable(m_apply);

  SendMessage(m_tabs, CB_ADDSTRING, 0, (LPARAM)AUTO_STR("All"));
  SendMessage(m_tabs, CB_ADDSTRING, 0, (LPARAM)AUTO_STR("Queued"));
  SendMessage(m_tabs, CB_ADDSTRING, 0, (LPARAM)AUTO_STR("Installed"));
  SendMessage(m_tabs, CB_ADDSTRING, 0, (LPARAM)AUTO_STR("Out of date"));
  SendMessage(m_tabs, CB_ADDSTRING, 0, (LPARAM)AUTO_STR("Obsolete"));
  SendMessage(m_tabs, CB_ADDSTRING, 0, (LPARAM)AUTO_STR("Uninstalled"));
  SendMessage(m_tabs, CB_SETCURSEL, 0, 0);

  m_list = createControl<ListView>(IDC_PACKAGES, ListView::Columns{
    {AUTO_STR(""), 23},
    {AUTO_STR("Package Name"), 380},
    {AUTO_STR("Category"), 150},
    {AUTO_STR("Version"), 80},
    {AUTO_STR("Author"), 95},
    {AUTO_STR("Type"), 70},
  });

  m_list->onActivate([=] { history(m_list->itemUnderMouse()); });
  m_list->sortByColumn(1);

  refresh();

  m_filterTimer = startTimer(200);
}

void Browser::onCommand(const int id, const int event)
{
  namespace arg = std::placeholders;

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
  case ACTION_LATEST:
    installLatest(m_currentIndex);
    break;
  case ACTION_LATEST_ALL:
    selectionDo(bind(&Browser::installLatest, this, arg::_1, false));
    break;
  case ACTION_REINSTALL:
    reinstall(m_currentIndex);
    break;
  case ACTION_REINSTALL_ALL:
    selectionDo(bind(&Browser::reinstall, this, arg::_1, false));
    break;
  case ACTION_UNINSTALL:
    uninstall(m_currentIndex);
    break;
  case ACTION_UNINSTALL_ALL:
    selectionDo(bind(&Browser::uninstall, this, arg::_1, false));
    break;
  case ACTION_CONTENTS:
    contents(m_currentIndex);
    break;
  case ACTION_HISTORY:
    history(m_currentIndex);
    break;
  case ACTION_ABOUT:
    about(m_currentIndex);
    break;
  case ACTION_RESET_ALL:
    selectionDo(bind(&Browser::resetAction, this, arg::_1));
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
      apply();

      if(id == IDAPPLY)
        break;
    }
    else
      break;
  case IDCANCEL:
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

void Browser::onTimer(const int id)
{
  if(id == m_filterTimer)
    checkFilter();
}

void Browser::onContextMenu(HWND target, const int x, const int y)
{
  if(target != m_list->handle())
    return;

  SetFocus(m_list->handle());

  m_currentIndex = m_list->itemUnderMouse();
  const Entry *entry = getEntry(m_currentIndex);

  Menu menu;

  if(m_list->selectionSize() > 1) {
    menu.addAction(AUTO_STR("&Install/update selection"), ACTION_LATEST_ALL);
    menu.addAction(AUTO_STR("&Reinstall selection"), ACTION_REINSTALL_ALL);
    menu.addAction(AUTO_STR("&Uninstall selection"), ACTION_UNINSTALL_ALL);
    menu.addAction(AUTO_STR("&Clear queued action"), ACTION_RESET_ALL);
    menu.addSeparator();
  }

  if(!entry) {
    menu.addAction(AUTO_STR("&Select all"), IDC_SELECT);
    menu.addAction(AUTO_STR("&Unselect all"), IDC_UNSELECT);
    menu.show(x, y, handle());
    return;
  }

  if(entry->test(InstalledFlag)) {
    if(entry->test(OutOfDateFlag)) {
      auto_char installLabel[255] = {};
      auto_snprintf(installLabel, sizeof(installLabel), AUTO_STR("U&pdate to v%s"),
        make_autostring(entry->latest->name()).c_str());

      const UINT actionIndex = menu.addAction(installLabel, ACTION_LATEST);
      if(isTarget(entry, entry->latest))
        menu.check(actionIndex);
    }

    auto_char reinstallLabel[255] = {};
    auto_snprintf(reinstallLabel, sizeof(reinstallLabel), AUTO_STR("&Reinstall v%s"),
      make_autostring(entry->regEntry.version.name()).c_str());

    const UINT actionIndex = menu.addAction(reinstallLabel, ACTION_REINSTALL);
    if(!entry->current || entry->test(ObsoleteFlag))
      menu.disable(actionIndex);
    else if(isTarget(entry, entry->current))
      menu.check(actionIndex);
  }
  else {
    auto_char installLabel[255] = {};
    auto_snprintf(installLabel, sizeof(installLabel), AUTO_STR("&Install v%s"),
      make_autostring(entry->latest->name()).c_str());

    const UINT actionIndex = menu.addAction(installLabel, ACTION_LATEST);
    if(isTarget(entry, entry->latest))
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

      const bool isAction = hasAction(entry);
      if(isAction ? isTarget(entry, ver) : ver == entry->current) {
        if(isAction && ver != entry->latest)
          menu.check(versionMenuIndex);

        versionMenu.checkRadio(actionIndex);
      }
    }
  }

  const UINT uninstallIndex =
    menu.addAction(AUTO_STR("&Uninstall"), ACTION_UNINSTALL);
  if(!entry->test(InstalledFlag))
    menu.disable(uninstallIndex);
  else if(isTarget(entry, nullptr))
    menu.check(uninstallIndex);

  menu.addSeparator();

  menu.setEnabled(!entry->test(ObsoleteFlag),
    menu.addAction(AUTO_STR("Package &Contents"), ACTION_CONTENTS));

  menu.setEnabled(!entry->test(ObsoleteFlag),
    menu.addAction(AUTO_STR("Package &History"), ACTION_HISTORY));

  auto_char aboutLabel[255] = {};
  const auto_string &name = make_autostring(getValue(RemoteColumn, *entry));
  auto_snprintf(aboutLabel, sizeof(aboutLabel),
    AUTO_STR("&About %s..."), name.c_str());
  menu.addAction(aboutLabel, ACTION_ABOUT);

  menu.show(x, y, handle());
}

void Browser::displayButton()
{
  RECT rect;
  GetWindowRect(m_display, &rect);

  static map<const auto_char *, Package::Type> types = {
    {AUTO_STR("&Scripts"), Package::ScriptType},
    {AUTO_STR("&Effects"), Package::EffectType},
    {AUTO_STR("E&xtensions"), Package::ExtensionType},
    {AUTO_STR("&Other packages"), Package::UnknownType},
  };

  Menu menu;
  for(const auto &pair : types) {
    const auto index = menu.addAction(pair.first,
      pair.second | (ACTION_FILTERTYPE << 8));

    if(!isFiltered(pair.second))
      menu.check(index);
  }

  menu.addSeparator();
  menu.addAction(AUTO_STR("Re&fresh repositories"), ACTION_REFRESH);
  menu.addAction(AUTO_STR("&Manage repositories..."), ACTION_MANAGE);

  menu.show(rect.left, rect.bottom - 1, handle());
}

bool Browser::isFiltered(Package::Type type) const
{
  switch(type) {
  case Package::ScriptType:
  case Package::EffectType:
  case Package::ExtensionType:
    break;
  default:
    type = Package::UnknownType;
  }

  auto config = m_reapack->config()->browser();
  return ((config->typeFilter >> type) & 1) == 1;
}

void Browser::toggleFiltered(const Package::Type type)
{
  auto config = m_reapack->config()->browser();
  config->typeFilter ^= 1 << type;
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
    m_loading = false;

    // keep the old indexes around a little bit longer for use by populate
    indexes.swap(m_indexes);

    populate();

    if(!m_loaded) {
      m_loaded = true;
      show();

#ifdef LVSCW_AUTOSIZE_USEHEADER
       m_list->resizeColumn(m_list->columnCount() - 1, LVSCW_AUTOSIZE_USEHEADER);
#endif
    }
  }, handle(), stale);
}

void Browser::populate()
{
  try {
    Registry reg(Path::prefixRoot(Path::REGISTRY));

    std::vector<Entry> oldEntries; // keep old entries in memory a bit longer
    swap(m_entries, oldEntries);

    m_currentIndex = -1;
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
    auto_snprintf(msg, sizeof(msg),
      AUTO_STR("ReaPack could not read from its package registry.\r\n")
      AUTO_STR("Retry later once all installation task are completed.\r\n")
      AUTO_STR("\r\nError description: %s"),
      desc.c_str());
    MessageBox(handle(), msg, AUTO_STR("ReaPack"), MB_OK);
  }
}

void Browser::transferActions()
{
  auto actionIt = m_actions.begin();
  while(actionIt != m_actions.end())
  {
    const Entry &oldEntry = *actionIt->first;
    const Version *target = actionIt->second;

    const auto &entryIt = find(m_entries.begin(), m_entries.end(), oldEntry);

    actionIt = m_actions.erase(actionIt);

    if(entryIt == m_entries.end())
      continue;

    if(target) {
      const Package *pkg = entryIt->package;
      if(!pkg || !(target = pkg->findVersion(*target)))
        continue;
    }

    m_actions[&*entryIt] = target;
  }
}

auto Browser::makeEntry(const Package *pkg, const Registry::Entry &regEntry)
  const -> Entry
{
  const auto &instOpts = *m_reapack->config()->install();
  const bool includePre = instOpts.bleedingEdge || !regEntry.version.isStable();
  const Version *latest = pkg->lastVersion(includePre);
  const Version *current = nullptr;

  int flags = 0;

  if(regEntry) {
    flags |= InstalledFlag;

    if(latest && regEntry.version < *latest)
      flags |= OutOfDateFlag;

    current = pkg->findVersion(regEntry.version);
  }
  else {
    if(!latest) // show prerelases if no stable version is available
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
  const auto selection = m_list->selection();
  vector<size_t> selected(min(selection.size(), m_visibleEntries.size()));
  for(size_t i = 0; i < selected.size(); i++) {
    if(i < m_visibleEntries.size())
      selected[i] = m_visibleEntries[selection[i]];
  }

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

  basic_ostringstream<auto_char> btnLabel;
  btnLabel.imbue(locale("")); // enable number formatting
  btnLabel << m_list->rowCount() << AUTO_STR('/') << m_entries.size()
    << AUTO_STR(" package");

  if(m_entries.size() != 1)
    btnLabel << AUTO_STR('s');

  SetWindowText(m_display, btnLabel.str().c_str());
}

ListView::Row Browser::makeRow(const Entry &entry) const
{
  const string &state = getValue(StateColumn, entry);
  const string &name = getValue(NameColumn, entry);
  const string &category = getValue(CategoryColumn, entry);
  const string &version = getValue(VersionColumn, entry);
  const string &author = getValue(AuthorColumn, entry);
  const string &type = getValue(TypeColumn, entry);

  return {
    make_autostring(state), make_autostring(name), make_autostring(category),
    make_autostring(version), make_autostring(author), make_autostring(type)
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
    else if(entry.test(OutOfDateFlag))
      display += 'u';
    else if(entry.test(InstalledFlag))
      display += 'i';
    else
      display += '\x20';

    if(hasAction(&entry))
      display += isTarget(&entry, nullptr) ? 'U' : 'I';

    return display;
  }
  case NameColumn:
    return pkg ? pkg->name() : regEntry.package;
  case CategoryColumn:
    return pkg ? pkg->category()->name() : regEntry.category;
  case VersionColumn:
    if(entry.test(InstalledFlag))
      display = regEntry.version.name();

    if(ver && *ver != regEntry.version) {
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
  }

  return {}; // for MSVC
}

bool Browser::match(const Entry &entry) const
{
  using namespace boost;

  switch(currentTab()) {
  case All:
    break;
  case Queued:
    if(!hasAction(&entry))
      return false;
    break;
  case Installed:
    if(!entry.test(InstalledFlag))
      return false;
    break;
  case OutOfDate:
    if(!entry.test(OutOfDateFlag))
      return false;
    break;
  case Uninstalled:
    if(!entry.test(UninstalledFlag))
      return false;
    break;
  case Obsolete:
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

  return m_filter.match(name + category + author);
}

auto Browser::getEntry(const int listIndex) const -> const Entry *
{
  if(listIndex < 0 || listIndex >= (int)m_visibleEntries.size())
    return nullptr;

  return &m_entries[m_visibleEntries[listIndex]];
}

void Browser::history(const int index) const
{
  const Entry *entry = getEntry(index);

  if(entry && entry->package)
    Dialog::Show<History>(instance(), handle(), entry->package);
}

void Browser::contents(const int index) const
{
  const Entry *entry = getEntry(index);

  if(entry)
    Dialog::Show<Contents>(instance(), handle(), entry->package);
}

void Browser::about(const int index) const
{
  if(const Entry *entry = getEntry(index))
    m_reapack->about(getValue(RemoteColumn, *entry), handle());
}

void Browser::installLatest(const int index, const bool toggle)
{
  const Entry *entry = getEntry(index);

  if(entry && entry->latest && entry->latest != entry->current)
    setAction(index, entry->latest, toggle);
}

void Browser::reinstall(const int index, const bool toggle)
{
  const Entry *entry = getEntry(index);

  if(entry && entry->current)
    setAction(index, entry->current, toggle);
}

void Browser::installVersion(const int index, const size_t verIndex)
{
  const Entry *entry = getEntry(index);
  const auto versions = entry->package->versions();

  if(verIndex >= versions.size())
    return;

  const Version *target = entry->package->version(verIndex);

  if(target == entry->current)
    resetAction(index);
  else
    setAction(index, target);
}

void Browser::uninstall(const int index, const bool toggle)
{
  const Entry *entry = getEntry(index);

  if(entry && entry->test(InstalledFlag))
    setAction(index, nullptr, toggle);
}

void Browser::resetAction(const int index)
{
  const Entry *entry = getEntry(index);
  const auto it = m_actions.find(entry);

  if(it == m_actions.end())
    return;

  m_actions.erase(it);

  if(currentTab() == Queued) {
    m_list->removeRow(index);
    m_visibleEntries.erase(m_visibleEntries.begin() + index);
  }
  else
    m_list->replaceRow(index, makeRow(*entry));

  if(m_actions.empty())
    disable(m_apply);
}

bool Browser::isTarget(const Entry *entry, const Version *target) const
{
  const auto it = m_actions.find(entry);

  if(it == m_actions.end())
    return false;
  else
    return it->second == target;
}

void Browser::setAction(const int index, const Version *target, const bool toggle)
{
  const Entry *entry = getEntry(index);

  if(toggle && isTarget(entry, target))
    resetAction(index);
  else if(entry) {
    m_actions[entry] = target;
    m_list->replaceRow(index, makeRow(*entry));
    enable(m_apply);
  }
}

void Browser::selectionDo(const std::function<void (int)> &func)
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
    swap(newSize, lastSize);
  }
}

auto Browser::currentTab() const -> Tab
{
  return (Tab)SendMessage(m_tabs, CB_GETCURSEL, 0, 0);
}

bool Browser::confirm() const
{
  if(m_actions.empty())
    return true;

  const size_t count = m_actions.size();

  auto_char msg[255] = {};
  auto_snprintf(msg, sizeof(msg),
    AUTO_STR("Confirm execution of %zu action%s?\n"),
    count, count == 1 ? AUTO_STR("") : AUTO_STR("s"));

  const auto_char *title = AUTO_STR("ReaPack Query");
  const int btn = MessageBox(handle(), msg, title, MB_YESNO);

  return btn == IDYES;
}

void Browser::apply()
{
  if(m_actions.empty())
    return;

  disable(m_apply);

  for(const auto &pair : m_actions) {
    if(pair.second)
      m_reapack->install(pair.second);
    else
      m_reapack->uninstall(pair.first->regEntry);
  }

  m_actions.clear();
  m_reapack->runTasks();

  if(m_reapack->isRunning())
    fillList(); // update state column
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
