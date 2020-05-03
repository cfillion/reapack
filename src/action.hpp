/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2020  Christian Fillion
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

#ifndef REAPACK_ACTION_HPP

#include <functional>
#include <map>
#include <memory>

#include <reaper_plugin.h>

class Action {
public:
  typedef unsigned short CommandID;
  typedef std::function<void ()> Callback;

  Action(const char *name, const char *desc, const Callback &);
  Action(const Action &) = delete;
  ~Action();

  CommandID id() const { return m_reg.accel.cmd; }
  void run() const { m_callback(); }

private:
  const char *m_name;
  gaccel_register_t m_reg;
  Callback m_callback;
};

class ActionList {
public:
  template<typename... Args> void add(Args&&... args)
  {
    auto action = std::make_unique<Action>(args...);
    m_list.emplace(action->id(), std::move(action));
  }

  bool run(Action::CommandID id) const;

private:
  Action *find(Action::CommandID id) const;

  std::map<Action::CommandID, std::unique_ptr<Action>> m_list;
};

#endif
