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

#include "receipt.hpp"

#include "transaction.hpp"

Receipt::Receipt()
  : m_enabled(false), m_needRestart(false)
{
}

void Receipt::addTicket(const InstallTicket &ticket)
{
  if(ticket.regQuery.status == Registry::UpdateAvailable)
    m_updates.push_back(ticket);
  else
    m_installs.push_back(ticket);
}

void Receipt::addRemovals(const std::set<Path> &pathList)
{
  m_removals.insert(pathList.begin(), pathList.end());
}
