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
#include "reapack.hpp"
#include "resource.hpp"

#include <boost/algorithm/string.hpp>

using namespace std;

Browser::Browser(const vector<IndexPtr> &indexes, ReaPack *reapack)
  : Dialog(IDD_BROWSER_DIALOG), m_indexes(indexes), m_reapack(reapack),
    m_checkFilter(false)
{
}

void Browser::onInit()
{
  m_filterHandle = getControl(IDC_FILTER);
  m_display = getControl(IDC_DISPLAY);

  SendMessage(m_display, CB_ADDSTRING, 0, (LPARAM)AUTO_STR("All"));
  SendMessage(m_display, CB_ADDSTRING, 0, (LPARAM)AUTO_STR("Installed"));
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
  case IDOK:
  case IDCANCEL:
    close();
    break;
  }
}

void Browser::onContextMenu(HWND, const int, const int)
{
  (void)m_reapack;
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

    m_entries.clear();

    for(IndexPtr index : m_indexes) {
      for(const Package *pkg : index->packages())
        m_entries.push_back({pkg->lastVersion(), reg.getEntry(pkg)});
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

  for(const Entry &entry : m_entries) {
    if(match(entry))
      m_list->addRow(makeRow(entry));
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
  const Version *ver = entry.version;
  const Package *pkg = ver ? ver->package() : nullptr;
  const Registry::Entry &regEntry = entry.regEntry;

  string display;

  switch(col) {
  case StateColumn:
    if(regEntry.id)
      display += ver ? 'i' : 'o';
    else
      display += '\x20';

    return display;
  case NameColumn:
    return pkg ? pkg->name() : regEntry.package;
  case CategoryColumn:
    return pkg ? pkg->category()->name() : regEntry.category;
  case VersionColumn:
    if(regEntry.id)
      display = regEntry.versionName;

    if(ver && ver->code() != regEntry.versionCode) {
      if(!display.empty())
        display += "\x20";

      display += "(" + ver->name() + ")";
    }

    return display;
  case AuthorColumn:
    return ver ? ver->displayAuthor() : "";
  case TypeColumn:
    return pkg ? pkg->displayType() : Package::displayType(regEntry.type);
  }
}

bool Browser::match(const Entry &entry) const
{
  using namespace boost;

  int display = SendMessage(m_display, CB_GETCURSEL, 0, 0);
  switch(display) {
  case 1: // Installed
    if(!entry.regEntry.id)
      return false;
    break;
  case 2: // Uninstalled
    if(entry.regEntry.id)
      return false;
    break;
  case 3: // Obsolete
    if(entry.version)
      return false;
    break;
  }

  const Package::Type type =
    entry.version ? entry.version->package()->type() : entry.regEntry.type;

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
