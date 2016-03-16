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
#include "index.hpp"
#include "listview.hpp"
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
  m_reloadTimer = startTimer(200);
}

void Browser::onCommand(const int id)
{
  switch(id) {
  case IDC_SCRIPTS:
  case IDC_EFFECTS:
  case IDC_EXTENSIONS:
    reload();
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
  if(id != m_reloadTimer || !m_checkFilter)
    return;

  checkFilter();
}

void Browser::checkFilter()
{
  m_checkFilter = false;

  auto_string wideFilter(4096, 0);
  GetWindowText(m_filterHandle, &wideFilter[0], (int)wideFilter.size());
  wideFilter.resize(wideFilter.find(AUTO_STR('\0')));

  const string &filter = from_autostring(wideFilter);

  if(filter != m_filter) {
    m_filter = filter;
    reload();
  }
}

void Browser::reload()
{
  InhibitControl freeze(m_list);

  m_list->clear();

  for(IndexPtr index : m_indexes) {
    for(const Package *pkg : index->packages()) {
      const Version *ver = pkg->lastVersion();

      if(!match(ver))
        continue;

      m_list->addRow({"??", pkg->name(), pkg->category()->name(),
        ver->name(), ver->displayAuthor(), pkg->displayType()});
    }
  }

  m_list->sort();
}

bool Browser::match(const Version *ver)
{
  using namespace boost;

  const Package *pkg = ver->package();

  const auto typeIt = m_types.find(pkg->type());

  if(typeIt == m_types.end() ||
      SendMessage(typeIt->second, BM_GETCHECK, 0, 0) == BST_UNCHECKED)
    return false;

  return icontains(pkg->name(), m_filter) ||
    icontains(pkg->category()->name(), m_filter) ||
    icontains(ver->author(), m_filter);
}
