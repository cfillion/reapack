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

#include "remote.hpp"

using namespace std;

Remote::Remote()
  : m_enabled(true), m_frozen(false)
{
}

Remote::Remote(const string &name, const string &url, const bool enabled)
  : m_name(name), m_url(url), m_enabled(enabled), m_frozen(false)
{
}

void RemoteList::add(const Remote &remote)
{
  if(remote.isNull())
    return;

  m_remotes[remote.name()] = remote;
}

Remote RemoteList::get(const string &name) const
{
  const auto it = m_remotes.find(name);

  if(it == m_remotes.end())
    return {};
  else
    return it->second;
}

bool RemoteList::hasUrl(const string &url) const
{
  for(const Remote &r : m_remotes | boost::adaptors::map_values) {
    if(r.url() == url)
      return true;
  }

  return false;
}
