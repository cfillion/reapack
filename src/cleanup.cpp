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

#include "cleanup.hpp"

#include "encoding.hpp"
#include "errors.hpp"
#include "index.hpp"
#include "listview.hpp"
#include "menu.hpp"
#include "reapack.hpp"
#include "resource.hpp"

using namespace std;

enum { ACTION_SELECT = 300, ACTION_UNSELECT };

Cleanup::Cleanup(const std::vector<IndexPtr> &indexes, ReaPack *reapack)
  : Dialog(IDD_CLEANUP_DIALOG), m_indexes(indexes), m_reapack(reapack)
{
}

void Cleanup::onInit()
{
  (void)m_reapack;
  m_ok = getControl(IDOK);
  m_stateLabel = getControl(IDC_LABEL2);

  disable(m_ok);

  m_list = createControl<ListView>(IDC_LIST, ListView::Columns{
    {AUTO_STR("Name"), 560},
  });

  m_list->sortByColumn(0);
  m_list->onSelect(bind(&Cleanup::onSelectionChanged, this));

  try {
    populate();
  }
  catch(const reapack_error &e) {
    const auto_string &desc = make_autostring(e.what());
    auto_char msg[255] = {};
    auto_snprintf(msg, sizeof(msg),
      AUTO_STR("The file list is currently unavailable.\x20")
      AUTO_STR("Retry later when all installation task are completed.\r\n")
      AUTO_STR("\r\nError description: %s"),
      desc.c_str());
    MessageBox(handle(), msg, AUTO_STR("AAA"), MB_OK);
  }

#ifdef LVSCW_AUTOSIZE_USEHEADER
  m_list->resizeColumn(0, LVSCW_AUTOSIZE_USEHEADER);
#endif

  onSelectionChanged();

  if(m_list->empty())
    startTimer(10);
}

void Cleanup::onCommand(const int id)
{
  switch(id) {
  case ACTION_SELECT:
    m_list->selectAll();
    break;
  case ACTION_UNSELECT:
    m_list->unselectAll();
    break;
  case IDOK:
    if(confirm())
      apply();
    else
      break;
  case IDCANCEL:
    close();
    break;
  }
}

void Cleanup::onContextMenu(HWND target, const int x, const int y)
{
  if(target != m_list->handle())
    return;

  Menu menu;
  menu.addAction(AUTO_STR("&Select all"), ACTION_SELECT);
  menu.addAction(AUTO_STR("&Unselect all"), ACTION_UNSELECT);
  menu.show(x, y, handle());
}

void Cleanup::onTimer(const int id)
{
  stopTimer(id);

  MessageBox(handle(), AUTO_STR("No obsolete package found!"),
    AUTO_STR("ReaPack"), MB_OK);

  close();
}

void Cleanup::populate()
{
  Registry reg(Path::prefixRoot(Path::REGISTRY));

  for(IndexPtr index : m_indexes) {
    for(const Registry::Entry &entry : reg.getEntries(index->name())) {
      const Category *cat = index->category(entry.category);

      if(cat && cat->package(entry.package))
        continue;

      const string row = entry.remote + "/" + entry.category + "/" + entry.package;
      m_list->addRow({make_autostring(row)});

      m_entries.push_back(entry);
    }
  }

  m_list->sort();
}

void Cleanup::onSelectionChanged()
{
  const int selectionSize = m_list->selectionSize();

  auto_char state[255] = {};
  auto_snprintf(state, sizeof(state), AUTO_STR("%d of %d package%s selected"),
    selectionSize, m_list->rowCount(),
    selectionSize == 1 ? AUTO_STR("") : AUTO_STR("s"));

  SetWindowText(m_stateLabel, state);

  setEnabled(selectionSize > 0, m_ok);
}

bool Cleanup::confirm() const
{
  const int count = m_list->selectionSize();

  auto_char msg[255] = {};
  auto_snprintf(msg, sizeof(msg),
    AUTO_STR("Uninstall %d package%s?\n")
    AUTO_STR("Every file they contain will be removed from your computer."),
    count, count == 1 ? AUTO_STR("") : AUTO_STR("s"));

  const auto_char *title = AUTO_STR("ReaPack Query");
  const int btn = MessageBox(handle(), msg, title, MB_YESNO);

  return btn == IDYES;
}

void Cleanup::apply()
{
  for(const int i : m_list->selection())
    m_reapack->uninstall(m_entries[i]);

  m_reapack->runTasks();
}
