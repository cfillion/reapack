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

enum Action { ACTION_HISTORY = 300, ACTION_ABOUT };

Browser::Browser(const vector<IndexPtr> &indexes, ReaPack *reapack)
  : Dialog(IDD_BROWSER_DIALOG), m_indexes(indexes), m_reapack(reapack),
    m_checkFilter(false), m_currentEntry(nullptr)
{
}

void Browser::onInit()
{
  m_filterHandle = getControl(IDC_FILTER);
  m_display = getControl(IDC_DISPLAY);

  SendMessage(m_display, CB_ADDSTRING, 0, (LPARAM)AUTO_STR("All"));
  SendMessage(m_display, CB_ADDSTRING, 0, (LPARAM)AUTO_STR("Installed"));
  SendMessage(m_display, CB_ADDSTRING, 0, (LPARAM)AUTO_STR("Out of date"));
  SendMessage(m_display, CB_ADDSTRING, 0, (LPARAM)AUTO_STR("Uninstalled"));
  SendMessage(m_display, CB_ADDSTRING, 0, (LPARAM)AUTO_STR("Obsolete"));

  m_types = {
    {Package::ScriptType, getControl(IDC_SCRIPTS)},
    {Package::EffectType, getControl(IDC_EFFECTS)},
    {Package::ExtensionType, getControl(IDC_EXTENSIONS)},
  };

  for(const auto &pair : m_types)
    SendMessage(pair.second, BM_SETCHECK, BST_CHECKED, 0);

  m_list = createControl<ListView>(IDC_PACKAGES, ListView::Columns{
    {AUTO_STR(""), 20},
    {AUTO_STR("Package Name"), 382},
    {AUTO_STR("Category"), 150},
    {AUTO_STR("Version"), 80},
    {AUTO_STR("Author"), 105},
    {AUTO_STR("Type"), 70},
  });

  m_list->onActivate([=] { history(entryAt(m_list->itemUnderMouse())); });

  m_list->sortByColumn(1);

  reload();
  m_filterTimer = startTimer(200);
}

void Browser::onCommand(const int id)
{
  switch(id) {
  case IDC_DISPLAY:
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
  case ACTION_HISTORY:
    history(m_currentEntry);
    break;
  case ACTION_ABOUT:
    about(m_currentEntry);
    break;
  case IDOK:
  case IDCANCEL:
    close();
    break;
  }
}

void Browser::onContextMenu(HWND target, const int x, const int y)
{
  if(target != m_list->handle())
    return;

  SetFocus(m_list->handle());

  const Entry *entry = entryAt(m_list->itemUnderMouse());
  m_currentEntry = entry;

  if(!entry)
    return;

  Menu menu;

  if(entry->test(InstalledFlag)) {
    if(entry->test(OutOfDateFlag)) {
      auto_char installLabel[255] = {};
      auto_snprintf(installLabel, sizeof(installLabel),
        AUTO_STR("&Update to v%s"), make_autostring(entry->latest->name()).c_str());

      menu.addAction(installLabel, 0);
    }

    auto_char reinstallLabel[255] = {};
    auto_snprintf(reinstallLabel, sizeof(reinstallLabel),
      AUTO_STR("&Reinstall v%s"),
      make_autostring(entry->regEntry.versionName).c_str());

    menu.setEnabled(!entry->test(ObsoleteFlag),
      menu.addAction(reinstallLabel, 0));
  }
  else {
    auto_char installLabel[255] = {};
    auto_snprintf(installLabel, sizeof(installLabel),
      AUTO_STR("&Install v%s"), make_autostring(entry->latest->name()).c_str());

    menu.addAction(installLabel, 0);
  }

  Menu versions = menu.addMenu(AUTO_STR("Versions"));
  const UINT versionIndex = menu.size() - 1;
  if(entry->test(ObsoleteFlag))
    menu.disable(versionIndex);
  else {
    for(const Version *ver : entry->package->versions() | boost::adaptors::reversed)
      versions.addAction(make_autostring(ver->name()).c_str(), 0);
  }

  menu.setEnabled(entry->test(InstalledFlag),
    menu.addAction(AUTO_STR("&Uninstall"), 0));

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

void Browser::onTimer(const int id)
{
  if(id == m_filterTimer)
    checkFilter();
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

    m_currentEntry = nullptr;
    m_entries.clear();

    for(IndexPtr index : m_indexes) {
      for(const Package *pkg : index->packages()) {
        const Version *latest = pkg->lastVersion();
        const Registry::Entry &regEntry = reg.getEntry(pkg);
        int flags = 0;

        if(regEntry.id) {
          flags |= InstalledFlag;
          if(regEntry.versionCode < latest->code())
            flags |= OutOfDateFlag;
        }
        else
          flags |= UninstalledFlag;

        m_entries.push_back({flags, regEntry, pkg, latest});
      }

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
  case StateColumn:
    if(entry.test(ObsoleteFlag))
      display += 'o';
    else if(entry.test(OutOfDateFlag))
      display += 'u';
    else if(entry.test(InstalledFlag))
        display += 'i';
    else
      display += '\x20';

    return display;
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
    return ver ? ver->displayAuthor() : "";
  case TypeColumn:
    return pkg ? pkg->displayType() : Package::displayType(regEntry.type);
  case RemoteColumn:
    return pkg ? pkg->category()->index()->name() : regEntry.remote;
  }
}

bool Browser::match(const Entry &entry) const
{
  using namespace boost;

  enum Display { All, Installed, OutOfDate, Uninstalled, Obsolete };
  Display display = (Display)SendMessage(m_display, CB_GETCURSEL, 0, 0);

  switch(display) {
  case All:
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

auto Browser::entryAt(const int listIndex) const -> const Entry *
{
  if(listIndex < 0 || listIndex >= (int)m_visibleEntries.size())
    return nullptr;

  return &m_entries[m_visibleEntries[listIndex]];
}

void Browser::history(const Entry *entry) const
{
  if(entry && entry->package)
    Dialog::Show<History>(instance(), handle(), entry->package);
}

void Browser::about(const Entry *entry) const
{
  if(entry)
    m_reapack->about(getValue(RemoteColumn, *entry), handle());
}
