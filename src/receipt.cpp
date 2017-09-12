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

#include "receipt.hpp"

#include "index.hpp"

using namespace std;

Receipt::Receipt()
  : m_needRestart(false)
{
}

bool Receipt::empty() const
{
  return
    m_installs.empty() &&
    m_updates.empty() &&
    m_removals.empty() &&
    m_exports.empty() &&
    m_errors.empty();
}

void Receipt::addInstall(const InstallTicket &ticket)
{
  if(ticket.previous && ticket.previous.version < ticket.version->name())
    m_updates.push_back(ticket);
  else
    m_installs.push_back(ticket);

  m_indexes.insert(ticket.version->package()->category()
    ->index()->shared_from_this());
}

void Receipt::addRemovals(const set<Path> &pathList)
{
  m_removals.insert(pathList.begin(), pathList.end());
}
