/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2018  Christian Fillion
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
#include "win32.hpp"

#ifdef _WIN32
#  include <windows.h>
#else
#  include <swell/swell.h>
#endif

using namespace std;

static const char *GENERAL_GRP = "general";
static const char *VERSION_KEY = "version";

static const char *INSTALL_GRP = "install";
static const char *AUTOINSTALL_KEY = "autoinstall";
static const char *PRERELEASES_KEY = "prereleases";
static const char *PROMPTOBSOLETE_KEY = "promptobsolete";

static const char *ABOUT_GRP = "about";
static const char *MANAGER_GRP = "manager";

static const char *BROWSER_GRP = "browser";
static const char *STATE_KEY = "state";

static const char *NETWORK_GRP = "network";
static const char *PROXY_KEY = "proxy";
static const char *VERIFYPEER_KEY = "verifypeer";
static const char *STALETHRSH_KEY = "stalethreshold";

static const char *SIZE_KEY = "size";

static const char *REMOTES_GRP = "remotes";
static const char *REMOTE_KEY  = "remote";

inline static string nKey(const char *key, const unsigned int i)
{
  return key + to_string(i);
}

Config::Config(const Path &path)
  : m_path(path.join()), m_isFirstRun(false), m_version(0), m_remotesIniSize(0)
{
  resetOptions();
  read();
}

Config::~Config()
{
  write();
}

void Config::resetOptions()
{
  install = {false, false, true};
  network = {"", true, NetworkOpts::OneWeekThreshold};
  windowState = {};
}

void Config::restoreSelfRemote()
{
  const string name = "ReaPack";
  const string url = "https://reapack.com/index.xml";

  RemotePtr remote = remotes.getByName(name);

  if(remote)
    remote->setUrl(url);
  else {
    remote = make_shared<Remote>(name, url);
    remotes.add(remote);
  }

  remote->protect();
}

void Config::restoreDefaultRemotes()
{
  remotes.getSelf()->setEnabled(true);

  const pair<string, string> repos[] = {
    {"ReaTeam Scripts",
      "https://github.com/ReaTeam/ReaScripts/raw/master/index.xml"},
    {"ReaTeam JSFX",
      "https://github.com/ReaTeam/JSFX/raw/master/index.xml"},
    {"ReaTeam Themes",
      "https://github.com/ReaTeam/Themes/raw/master/index.xml"},
    {"ReaTeam LangPacks",
      "https://github.com/ReaTeam/LangPacks/raw/master/index.xml"},
    {"MPL Scripts",
      "https://github.com/MichaelPilyavskiy/ReaScripts/raw/master/index.xml"},
    {"X-Raym Scripts",
      "https://github.com/X-Raym/REAPER-ReaScripts/raw/master/index.xml"},
  };

  for(const auto &[name, url] : repos) {
    RemotePtr remote = remotes.getByName(name);

    if(remote)
      remote->setUrl(url);
    else {
      remote = make_shared<Remote>(name, url);
      remotes.add(remote);
    }

    remote->setEnabled(true);
  }
}

void Config::migrate()
{
  const unsigned int version = getUInt(GENERAL_GRP, VERSION_KEY);

  switch(version) {
  case 0: // v1.0
  case 1: // v1.1rc3
  case 2: // v1.1
    m_isFirstRun = true;
    restoreDefaultRemotes();
    break;
  default:
    // configuration is up-to-date, don't write anything now
    // keep the version intact in case it comes from a newer version of ReaPack
    m_version = version;
    return;
  };

  m_version = 3;
  write();
}

void Config::read()
{
  install.autoInstall = getBool(INSTALL_GRP, AUTOINSTALL_KEY, install.autoInstall);
  install.bleedingEdge = getBool(INSTALL_GRP, PRERELEASES_KEY, install.bleedingEdge);
  install.promptObsolete = getBool(INSTALL_GRP, PROMPTOBSOLETE_KEY, install.promptObsolete);

  network.proxy = getString(NETWORK_GRP, PROXY_KEY, network.proxy);
  network.verifyPeer = getBool(NETWORK_GRP, VERIFYPEER_KEY, network.verifyPeer);
  network.staleThreshold = (time_t)getUInt(NETWORK_GRP,
    STALETHRSH_KEY, (unsigned int)network.staleThreshold);

  windowState.about = getString(ABOUT_GRP, STATE_KEY, windowState.about);
  windowState.browser = getString(BROWSER_GRP, STATE_KEY, windowState.browser);
  windowState.manager = getString(MANAGER_GRP, STATE_KEY, windowState.manager);

  readRemotes();
  restoreSelfRemote();
  migrate();
}

void Config::write()
{
  setUInt(GENERAL_GRP, VERSION_KEY, m_version);

  setUInt(INSTALL_GRP, AUTOINSTALL_KEY, install.autoInstall);
  setUInt(INSTALL_GRP, PRERELEASES_KEY, install.bleedingEdge);
  setUInt(INSTALL_GRP, PROMPTOBSOLETE_KEY, install.promptObsolete);

  setString(NETWORK_GRP, PROXY_KEY, network.proxy);
  setUInt(NETWORK_GRP, VERIFYPEER_KEY, network.verifyPeer);
  setUInt(NETWORK_GRP, STALETHRSH_KEY, (unsigned int)network.staleThreshold);

  setString(ABOUT_GRP, STATE_KEY, windowState.about);
  setString(BROWSER_GRP, STATE_KEY, windowState.browser);
  setString(MANAGER_GRP, STATE_KEY, windowState.manager);


  writeRemotes();
}

void Config::readRemotes()
{
  m_remotesIniSize = getUInt(REMOTES_GRP, SIZE_KEY);

  for(unsigned int i = 0; i < m_remotesIniSize; i++) {
    const string &data = getString(REMOTES_GRP, nKey(REMOTE_KEY, i).c_str());
    remotes.add(Remote::fromString(data));
  }
}

void Config::writeRemotes()
{
  m_remotesIniSize = max((unsigned int)remotes.size(), m_remotesIniSize);

  unsigned int i = 0;
  for(auto it = remotes.begin(); it != remotes.end(); it++, i++)
    setString(REMOTES_GRP, nKey(REMOTE_KEY, i).c_str(), (*it)->toString());

  cleanupArray(REMOTES_GRP, REMOTE_KEY, i, m_remotesIniSize);

  setUInt(REMOTES_GRP, SIZE_KEY, m_remotesIniSize = i);
}

string Config::getString(const char *group, const char *key, const string &fallback) const
{
  return Win32::getPrivateProfileString(group, key, fallback.c_str(), m_path.c_str());
}

void Config::setString(const char *group, const char *key, const string &val) const
{
  Win32::writePrivateProfileString(group, key, val.c_str(), m_path.c_str());
}

unsigned int Config::getUInt(const char *group,
  const char *key, const unsigned int fallback) const
{
  return Win32::getPrivateProfileInt(group, key, fallback, m_path.c_str());
}

bool Config::getBool(const char *group, const char *key, const bool fallback) const
{
  return getUInt(group, key, fallback) > 0;
}

void Config::setUInt(const char *group, const char *key, const unsigned int val) const
{
  setString(group, key, to_string(val));
}

void Config::deleteKey(const char *group, const char *key) const
{
  Win32::writePrivateProfileString(group, key, nullptr, m_path.c_str());
}

void Config::cleanupArray(const char *group, const char *key,
  const unsigned int begin, const unsigned int end) const
{
  for(unsigned int i = begin; i < end; i++)
    deleteKey(group, nKey(key, i).c_str());
}
