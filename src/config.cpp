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

#include "config.hpp"

#include "path.hpp"

#ifdef _WIN32
#undef UNICODE
#include <windows.h>
#else
#include <swell/swell.h>
#endif

#include <reaper_plugin_functions.h>

using namespace std;

static const char *SIZE_KEY = "size";

static const char *REMOTES_GRP = "remotes";
static const char *REMOTE_KEY  = "remote";

static string ArrayKey(const string &key, const size_t i)
{
  return key + to_string(i);
}

static const int BUFFER_SIZE = 2083;

Config::Config()
  : m_remotesIniSize(0)
{
}

void Config::fillDefaults()
{
  m_remotes.add({"ReaTeam Scripts",
    "https://github.com/ReaTeam/ReaScripts/raw/master/index.xml", false});
}

void Config::read(const Path &path)
{
  m_path = path.join();

  if(!file_exists(m_path.c_str())) {
    fillDefaults();
    write();
  }
  else
    readRemotes();

  restoreSelfRemote();
}

void Config::write()
{
  writeRemotes();
}

void Config::restoreSelfRemote()
{
  const char *name = "ReaPack";
  const char *url = "https://github.com/cfillion/reapack/raw/master/index.xml";

  Remote remote = m_remotes.get(name);
  remote.setName(name);
  remote.setUrl(url);
  remote.protect();

  m_remotes.add(remote);
}

void Config::readRemotes()
{
  m_remotesIniSize = getUInt(REMOTES_GRP, SIZE_KEY);

  for(size_t i = 0; i < m_remotesIniSize; i++) {
    const string data = getString(REMOTES_GRP, ArrayKey(REMOTE_KEY, i));

    m_remotes.add(Remote::fromString(data));
  }
}

void Config::writeRemotes()
{
  size_t i = 0;
  m_remotesIniSize = max(m_remotes.size(), m_remotesIniSize);

  for(auto it = m_remotes.begin(); it != m_remotes.end(); it++, i++) {
    setString(REMOTES_GRP, ArrayKey(REMOTE_KEY, i), it->toString());
  }

  cleanupArray(REMOTES_GRP, REMOTE_KEY, i, m_remotesIniSize);

  setUInt(REMOTES_GRP, SIZE_KEY, m_remotesIniSize = i);
}

string Config::getString(const char *group, const string &key) const
{
  char buffer[BUFFER_SIZE];
  GetPrivateProfileString(group, key.c_str(), "",
    buffer, sizeof(buffer), m_path.c_str());

  return buffer;
}

void Config::setString(const char *group,
  const string &key, const string &val) const
{
  WritePrivateProfileString(group, key.c_str(), val.c_str(), m_path.c_str());
}

size_t Config::getUInt(const char *group, const string &key) const
{
  const int i = GetPrivateProfileInt(group, key.c_str(), 0, m_path.c_str());
  return max(0, i);
}

void Config::setUInt(const char *grp, const string &key, const size_t val) const
{
  setString(grp, key, to_string(val));
}

void Config::deleteKey(const char *group, const string &key) const
{
  WritePrivateProfileString(group, key.c_str(), 0, m_path.c_str());
}

void Config::cleanupArray(const char *group, const string &key,
  const size_t begin, const size_t end) const
{
  for(size_t i = begin; i < end; i++)
    deleteKey(group, ArrayKey(key, i));
}
