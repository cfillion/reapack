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

#include "query.hpp"

#include "encoding.hpp"
#include "listview.hpp"
#include "menu.hpp"
#include "package.hpp"
#include "resource.hpp"

#include <sstream>

using namespace std;

enum { ACTION_SELECT_ALL = 300, ACTION_UNSELECT_ALL };

ObsoleteQuery::ObsoleteQuery(vector<Registry::Entry> *entries, bool *enable)
  : Dialog(IDD_QUERY_DIALOG), m_entries(entries), m_enable(enable)
{
}

void ObsoleteQuery::onInit()
{
  Dialog::onInit();

  m_enableCtrl = getControl(IDC_ENABLE);
  m_okBtn = getControl(IDOK);

  m_list = createControl<ListView>(IDC_LIST, ListView::Columns{
    {AUTO_STR("Package"), 550}
  });

  m_list->onSelect([=] { setEnabled(m_list->hasSelection(), m_okBtn); });
  m_list->onContextMenu([=] (Menu &menu, int) {
    menu.addAction(AUTO_STR("Select &all"), ACTION_SELECT_ALL);
    menu.addAction(AUTO_STR("&Unselect all"), ACTION_UNSELECT_ALL);
    return true;
  });

  for(const Registry::Entry &entry : *m_entries) {
    ostringstream stream;
    stream << entry.remote << '/' << entry.category << '/'
      << Package::displayName(entry.package, entry.description);
    m_list->addRow({make_autostring(stream.str())});
  }

  m_list->autoSizeHeader();

  SendMessage(m_enableCtrl, BM_SETCHECK, BST_CHECKED, 0);

  disable(m_okBtn);
}

void ObsoleteQuery::onCommand(const int id, int event)
{
  switch(id) {
  case IDOK:
    prepare();
    break;
  case IDC_ENABLE:
    *m_enable = SendMessage(m_enableCtrl, BM_GETCHECK, 0, 0) == BST_CHECKED;
    break;
  case ACTION_SELECT_ALL:
    m_list->selectAll();
    break;
  case ACTION_UNSELECT_ALL:
    m_list->unselectAll();
    break;
  }

  Dialog::onCommand(id, event);
}

void ObsoleteQuery::prepare()
{
  vector<Registry::Entry> selected;

  for(int index : m_list->selection())
    selected.emplace_back(m_entries->at(index));

  m_entries->swap(selected);
}
