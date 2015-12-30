/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015  Christian Fillion
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

#include "manager.hpp"

#include "encoding.hpp"
#include "resource.hpp"

const auto_char *NAME = AUTO_STR("Name");
const auto_char *URL = AUTO_STR("URL");

Manager::Manager()
  : Dialog(IDD_CONFIG_DIALOG), m_list(0)
{
}

Manager::~Manager()
{
  delete m_list;
}

void Manager::onInit()
{
  m_list = new ListView({
    {AUTO_STR("Name"), 130},
    {AUTO_STR("URL"), 350},
  }, getItem(IDC_LIST));
}

void Manager::onCommand(WPARAM wParam, LPARAM)
{
  switch(LOWORD(wParam)) {
  case IDOK:
    apply();
  case IDCANCEL:
    hide();
    break;
  }
}

void Manager::refresh()
{
  m_list->clear();
  m_list->addRow({AUTO_STR("Hello"), AUTO_STR("http://hello.com/index.xml")});
  m_list->addRow({AUTO_STR("World"), AUTO_STR("http://world.com/index.xml")});
}

void Manager::apply()
{
}
