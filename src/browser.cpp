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

#include "encoding.hpp"
#include "errors.hpp"
#include "index.hpp"
#include "menu.hpp"
#include "reapack.hpp"
#include "report.hpp"
#include "resource.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/range/adaptor/reversed.hpp>

using namespace std;

enum Action {
  ACTION_VERSION = 80,
  ACTION_LATEST = 300,
  ACTION_LATEST_ALL,
  ACTION_REINSTALL,
  ACTION_REINSTALL_ALL,
  ACTION_UNINSTALL,
  ACTION_UNINSTALL_ALL,
  ACTION_HISTORY,
  ACTION_ABOUT,
  ACTION_RESET_ALL,
};

Browser::Browser(const vector<IndexPtr> &indexes, ReaPack *reapack)
  : Dialog(IDD_BROWSER_DIALOG), m_indexes(indexes), m_reapack(reapack),
    m_checkFilter(false), m_currentIndex(-1)
{
}

void Browser::onInit()
{
  m_action = getControl(IDC_ACTION);
  m_apply = getControl(IDAPPLY);
  m_filterHandle = getControl(IDC_FILTER);
  m_display = getControl(IDC_DISPLAY);

  disable(m_apply);

  SendMessage(m_display, CB_ADDSTRING, 0, (LPARAM)AUTO_STR("All"));
  SendMessage(m_display, CB_ADDSTRING, 0, (LPARAM)AUTO_STR("Queued"));
  SendMessage(m_display, CB_ADDSTRING, 0, (LPARAM)AUTO_STR("Installed"));
  SendMessage(m_display, CB_ADDSTRING, 0, (LPARAM)AUTO_STR("Out of date"));
  SendMessage(m_display, CB_ADDSTRING, 0, (LPARAM)AUTO_STR("Obsolete"));
  SendMessage(m_display, CB_ADDSTRING, 0, (LPARAM)AUTO_STR("Uninstalled"));
  SendMessage(m_display, CB_SETCURSEL, 0, 0);

  m_types = {
    {Package::ScriptType, getControl(IDC_SCRIPTS)},
    {Package::EffectType, getControl(IDC_EFFECTS)},
    {Package::ExtensionType, getControl(IDC_EXTENSIONS)},
  };

  for(const auto &pair : m_types)
    SendMessage(pair.second, BM_SETCHECK, BST_CHECKED, 0);

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

  reload();

#ifdef LVSCW_AUTOSIZE_USEHEADER
  m_list->resizeColumn(m_list->columnCount() - 1, LVSCW_AUTOSIZE_USEHEADER);
#endif

  m_filterTimer = startTimer(200);
}

void Browser::onCommand(const short id, const short event)
{
  namespace arg = std::placeholders;

  switch(id) {
  case IDC_DISPLAY:
    if(event != CBN_SELCHANGE)
      break;
  case IDC_SCRIPTS:
  case IDC_EFFECTS:
  case IDC_EXTENSIONS:
    fillList();
    break;
  case IDC_FILTER:
    m_checkFilter = true;
    break;
  case IDC_CLEAR:
    SetWindowText(m_filterHandle, AUTO_STR(""));
    checkFilter();
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
    selectionMenu();
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
  case ACTION_HISTORY:
    history(m_currentIndex);
    break;
  case ACTION_ABOUT:
    about(m_currentIndex);
    break;
  case ACTION_RESET_ALL:
    selectionDo(bind(&Browser::resetAction, this, arg::_1));
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

  if(!entry)
    return;

  Menu menu;

  if(entry->test(InstalledFlag)) {
    if(entry->test(OutOfDateFlag)) {
      auto_char installLabel[255] = {};
      auto_snprintf(installLabel, sizeof(installLabel), AUTO_STR("&Update to v%s"),
        make_autostring(entry->latest->name()).c_str());

      const UINT actionIndex = menu.addAction(installLabel, ACTION_LATEST);
      if(isTarget(entry, entry->latest))
        menu.check(actionIndex);
    }

    auto_char reinstallLabel[255] = {};
    auto_snprintf(reinstallLabel, sizeof(reinstallLabel), AUTO_STR("&Reinstall v%s"),
      make_autostring(entry->regEntry.versionName).c_str());

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
  const UINT versionIndex = menu.size() - 1;
  if(entry->test(ObsoleteFlag))
    menu.disable(versionIndex);
  else {
    const auto &versions = entry->package->versions();
    int verIndex = (int)versions.size();
    for(const Version *ver : versions | boost::adaptors::reversed) {
      const UINT actionIndex = versionMenu.addAction(
        make_autostring(ver->name()).c_str(), --verIndex | (ACTION_VERSION << 8));

      if(hasAction(entry) ? isTarget(entry, ver) : ver == entry->current)
        versionMenu.checkRadio(actionIndex);
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
    menu.addAction(AUTO_STR("Package &History"), ACTION_HISTORY));

  auto_char aboutLabel[255] = {};
  const auto_string &name = make_autostring(getValue(RemoteColumn, *entry));
  auto_snprintf(aboutLabel, sizeof(aboutLabel),
    AUTO_STR("&About %s..."), name.c_str());
  menu.addAction(aboutLabel, ACTION_ABOUT);

  menu.show(x, y, handle());
}

void Browser::selectionMenu()
{
  RECT rect;
  GetWindowRect(m_action, &rect);

  const Entry *entry = nullptr;
  if(m_list->selectionSize() == 1)
    entry = getEntry(m_list->currentIndex());

  Menu menu;

  menu.addAction(AUTO_STR("&Install/update selection"), ACTION_LATEST_ALL);
  menu.addAction(AUTO_STR("&Reinstall selection"), ACTION_REINSTALL_ALL);
  menu.addAction(AUTO_STR("&Uninstall selection"), ACTION_UNINSTALL_ALL);
  menu.addAction(AUTO_STR("&Clear queued action"), ACTION_RESET_ALL);

  if(!m_list->hasSelection())
    menu.disableAll();

  menu.show(rect.left, rect.bottom - 1, handle());
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

  if(filter != m_filter) {
    m_filter = filter;
    fillList();
  }
}

void Browser::reload()
{
  try {
    Registry reg(Path::prefixRoot(Path::REGISTRY));

    m_currentIndex = -1;
    m_actions.clear();
    m_entries.clear();

    for(IndexPtr index : m_indexes) {
      for(const Package *pkg : index->packages())
        m_entries.push_back(makeEntry(pkg, reg.getEntry(pkg)));

      // obsolete packages
      for(const Registry::Entry &entry : reg.getEntries(index->name())) {
        const Category *cat = index->category(entry.category);

        if(cat && cat->package(entry.package))
          continue;

        m_entries.push_back({InstalledFlag | ObsoleteFlag, entry});
      }
    }

    fillList();
  }
  catch(const reapack_error &e) {
    const auto_string &desc = make_autostring(e.what());
    auto_char msg[255] = {};
    auto_snprintf(msg, sizeof(msg),
      AUTO_STR("ReaPack could not open the package registry.\r\n")
      AUTO_STR("Retry later when all installation task are completed.\r\n")
      AUTO_STR("\r\nError description: %s"),
      desc.c_str());
    MessageBox(handle(), msg, AUTO_STR("ReaPack"), MB_OK);
  }
}

auto Browser::makeEntry(const Package *pkg, const Registry::Entry &regEntry)
  -> Entry
{
  const Version *latest = pkg->lastVersion();
  const Version *current = nullptr;

  int flags = 0;

  if(regEntry.id) {
    flags |= InstalledFlag;

    if(regEntry.versionCode < latest->code())
      flags |= OutOfDateFlag;

    for(const Version *ver : pkg->versions()) {
      if(ver->code() == regEntry.versionCode) {
        current = ver;
        break;
      }
    }
  }
  else
    flags |= UninstalledFlag;

  return {flags, regEntry, pkg, latest, current};
}

void Browser::fillList()
{
  InhibitControl freeze(m_list);

  m_list->clear();
  m_visibleEntries.clear();

  for(size_t i = 0; i < m_entries.size(); i++) {
    const Entry &entry = m_entries[i];

    if(!match(entry))
      continue;

    m_list->addRow(makeRow(entry));
    m_visibleEntries.push_back(i);
  }

  m_list->sort();
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
      display = regEntry.versionName;

    if(ver && ver->code() != regEntry.versionCode) {
      if(!display.empty())
        display += '\x20';

      display += '(' + ver->name() + ')';
    }

    return display;
  case AuthorColumn:
    return ver ? ver->displayAuthor() : regEntry.author;
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

  switch(getDisplay()) {
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

  const auto typeIt = m_types.find(type);

  if(typeIt == m_types.end() ||
      SendMessage(typeIt->second, BM_GETCHECK, 0, 0) == BST_UNCHECKED)
    return false;

  const string &name = getValue(NameColumn, entry);
  const string &category = getValue(CategoryColumn, entry);
  const string &author = getValue(AuthorColumn, entry);

  return icontains(name, m_filter) || icontains(category, m_filter) ||
    icontains(author, m_filter);
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

  if(getDisplay() == Queued)
    m_list->removeRow(index);
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
  for(const int index : m_list->selection())
    func(index);
}

auto Browser::getDisplay() const -> Display
{
  return (Display)SendMessage(m_display, CB_GETCURSEL, 0, 0);
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

  fillList(); // update state column
}
