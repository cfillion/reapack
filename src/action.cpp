/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2024  Christian Fillion
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

#include "action.hpp"

#include <reaper_plugin_functions.h>

Action::Action(const char *name, const char *desc, const Callback &callback)
  : m_name(name), m_reg{}, m_callback(callback)
{
  m_reg.accel.cmd = plugin_register("command_id", const_cast<char *>(m_name));
  m_reg.desc = desc;

  plugin_register("gaccel", &m_reg);
}

Action::~Action()
{
  plugin_register("-gaccel", &m_reg);
  plugin_register("-command_id", const_cast<char *>(m_name));
}

Action *ActionList::find(const Action::CommandID id) const
{
  const auto it = m_list.find(id);
  return it != m_list.end() ? it->second.get() : nullptr;
}

bool ActionList::run(const Action::CommandID id) const
{
  if(Action *action = find(id)) {
    action->run();
    return true;
  }

  return false;
}
