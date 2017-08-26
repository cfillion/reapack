/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2017  Christian Fillion
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

#include <cstdio>
 
#include <reaper_plugin_functions.h>

APIDef::APIDef(const APIFunc *func)
  : m_func(func)
{
  plugin_register(m_func->cKey, m_func->cImpl);
  plugin_register(m_func->reascriptKey, m_func->reascriptImpl);
  plugin_register(m_func->definitionKey, m_func->definition);
}

APIDef::~APIDef()
{
  unregister(m_func->cKey, m_func->cImpl);
  unregister(m_func->reascriptKey, m_func->reascriptImpl);
  unregister(m_func->definitionKey, m_func->definition);
}

void APIDef::unregister(const char *key, void *ptr)
{
  char buf[255];
  snprintf(buf, sizeof(buf), "-%s", key);
  plugin_register(buf, ptr);
}
