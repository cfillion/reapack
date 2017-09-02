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

#include "config.hpp"

#include "path.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <swell/swell.h>
#endif

using namespace std;

static const CharT *GENERAL_GRP = L("general");
static const CharT *VERSION_KEY = L("version");

static const CharT *INSTALL_GRP = L("install");
static const CharT *AUTOINSTALL_KEY = L("autoinstall");
static const CharT *PRERELEASES_KEY = L("prereleases");
static const CharT *PROMPTOBSOLETE_KEY = L("promptobsolete");

static const CharT *ABOUT_GRP = L("about");
static const CharT *MANAGER_GRP = L("manager");

static const CharT *BROWSER_GRP = L("browser");
static const CharT *STATE_KEY = L("state");

static const CharT *NETWORK_GRP = L("network");
static const CharT *PROXY_KEY = L("proxy");
static const CharT *VERIFYPEER_KEY = L("verifypeer");
static const CharT *STALETHRSH_KEY = L("stalethreshold");

static const CharT *SIZE_KEY = L("size");

static const CharT *REMOTES_GRP = L("remotes");
static const CharT *REMOTE_KEY  = L("remote");

static String ArrayKey(const String &key, const unsigned int i)
{
  return key + String(i);
}

static const int BUFFER_SIZE = 2083;

Config::Config()
  : m_isFirstRun(false), m_version(0), m_remotesIniSize(0)
{
  resetOptions();
}

void Config::resetOptions()
{
  install = {false, false, true};
  network = {"", true, NetworkOpts::OneWeekThreshold};
  windowState = {};
}

void Config::restoreSelfRemote()
{
  const String name = L("ReaPack");

  Remote remote = remotes.get(name);
  remote.setName(name);
  remote.setUrl(L("https://github.com/cfillion/reapack/raw/master/index.xml"));
  remote.protect();

  remotes.add(remote);
}

void Config::restoreDefaultRemotes()
{
  Remote self = remotes.get(L("ReaPack"));
  self.setEnabled(true);
  remotes.add(self);

  const Remote repos[] = {
    {L("ReaTeam Scripts"),
      L("https://github.com/ReaTeam/ReaScripts/raw/master/index.xml")},
    {L("ReaTeam JSFX"),
      L("https://github.com/ReaTeam/JSFX/raw/master/index.xml")},
    {L("ReaTeam Themes"),
      L("https://github.com/ReaTeam/Themes/raw/master/index.xml")},
    {L("ReaTeam LangPacks"),
      L("https://github.com/ReaTeam/LangPacks/raw/master/index.xml")},
    {L("MPL Scripts"),
      L("https://github.com/MichaelPilyavskiy/ReaScripts/raw/master/index.xml")},
    {L("X-Raym Scripts"),
      L("https://github.com/X-Raym/REAPER-ReaScripts/raw/master/index.xml")},
  };

  for(const Remote &repo : repos)
    remotes.add(repo);
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

void Config::read(const Path &path)
{
  m_path = path.join();

  install.autoInstall = getBool(INSTALL_GRP, AUTOINSTALL_KEY, install.autoInstall);
  install.bleedingEdge = getBool(INSTALL_GRP, PRERELEASES_KEY, install.bleedingEdge);
  install.promptObsolete = getBool(INSTALL_GRP,
    PROMPTOBSOLETE_KEY, install.promptObsolete);

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
    const String data = getString(REMOTES_GRP, ArrayKey(REMOTE_KEY, i));

    remotes.add(Remote::fromString(data));
  }
}

void Config::writeRemotes()
{
  m_remotesIniSize = max((unsigned int)remotes.size(), m_remotesIniSize);

  unsigned int i = 0;
  for(auto it = remotes.begin(); it != remotes.end(); it++, i++)
    setString(REMOTES_GRP, ArrayKey(REMOTE_KEY, i), it->toString());

  cleanupArray(REMOTES_GRP, REMOTE_KEY, i, m_remotesIniSize);

  setUInt(REMOTES_GRP, SIZE_KEY, m_remotesIniSize = i);
}

String Config::getString(const CharT *group,
  const String &key, const String &fallback) const
{
  CharT buffer[BUFFER_SIZE];
  GetPrivateProfileString(group, key.c_str(), fallback.c_str(),
    buffer, sizeof(buffer), m_path.c_str());

  return buffer;
}

void Config::setString(const CharT *group,
  const String &key, const String &val) const
{
  WritePrivateProfileString(group, key.c_str(), val.c_str(), m_path.c_str());
}

unsigned int Config::getUInt(const CharT *group,
  const String &key, const unsigned int fallback) const
{
  return GetPrivateProfileInt(group, key.c_str(), fallback, m_path.c_str());
}

bool Config::getBool(const CharT *group,
  const String &key, const bool fallback) const
{
  return getUInt(group, key, fallback) > 0;
}

void Config::setUInt(const CharT *group, const String &key,
  const unsigned int val) const
{
  setString(group, key, String(val));
}

void Config::deleteKey(const CharT *group, const String &key) const
{
  WritePrivateProfileString(group, key.c_str(), 0, m_path.c_str());
}

void Config::cleanupArray(const CharT *group, const String &key,
  const unsigned int begin, const unsigned int end) const
{
  for(unsigned int i = begin; i < end; i++)
    deleteKey(group, ArrayKey(key, i));
}
