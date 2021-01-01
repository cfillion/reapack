/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2021  Christian Fillion
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

#include "api.hpp"

#include <string>
 
#include <reaper_plugin_functions.h>

using namespace std::string_literals;

#define KEY(prefix) (prefix "_ReaPack_"s + m_func->name)

APIReg::APIReg(const APIFunc *func)
  : m_func(func),
    m_impl(KEY("API")), m_vararg(KEY("APIvararg")), m_help(KEY("APIdef"))
{
  registerFunc();
}

APIReg::~APIReg()
{
  m_impl.insert(m_impl.begin(), '-');
  m_vararg.insert(m_vararg.begin(), '-');
  m_help.insert(m_help.begin(), '-');

  registerFunc();
}

void APIReg::registerFunc() const
{
  plugin_register(m_impl.c_str(), m_func->cImpl);
  plugin_register(m_vararg.c_str(), m_func->reascriptImpl);
  plugin_register(m_help.c_str(), m_func->definition);
}
