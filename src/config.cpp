#include "config.hpp"

#include "path.hpp"

#include <swell/swell.h>

#include <reaper_plugin_functions.h>

using namespace std;

static const char *REMOTES_GRP = "remotes";
static const char *NAME_KEY    = "name";
static const char *URL_KEY     = "url";

static const char *REGISTRY_GRP = "registry";
static const char *PACK_KEY     = "reapack";
static const char *VER_KEY      = "version";

static string ArrayKey(const string &key, const size_t i)
{
  return key + to_string(i);
}

static const int BUFFER_SIZE = 2083;

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

void Config::fillDefaults()
{
  m_remotes.push_back({"ReaScripts",
    "https://github.com/ReaTeam/ReaScripts/raw/master/index.xml"});
}

void Config::read(const Path &path)
{
  m_path = path.join();

  if(!file_exists(m_path.c_str())) {
    fillDefaults();
    write();
    return;
  }

  readRemotes();
  readRegistry();
}

void Config::write() const
{
  writeRemotes();
  writeRegistry();
}

void Config::readRemotes()
{
  size_t i = 0;

  do {
    const string name = getString(REMOTES_GRP, ArrayKey(NAME_KEY, i));
    const string url = getString(REMOTES_GRP, ArrayKey(URL_KEY, i));

    if(name.empty() || url.empty())
      break;

    m_remotes.push_back({name, url});
  } while(++i);
}

void Config::writeRemotes() const
{
  const size_t size = m_remotes.size();

  for(size_t i = 0; i < size; i++) {
    const Remote &remote = m_remotes[i];

    setString(REMOTES_GRP, ArrayKey(NAME_KEY, i), remote.name());
    setString(REMOTES_GRP, ArrayKey(URL_KEY, i), remote.url());
  }
}

void Config::readRegistry()
{
  size_t i = 0;

  do {
    const string pack = getString(REGISTRY_GRP, ArrayKey(PACK_KEY, i));
    const string ver = getString(REGISTRY_GRP, ArrayKey(VER_KEY, i));

    if(pack.empty() || ver.empty())
      break;

    m_registry.push(pack, ver);
  } while(++i);
}

void Config::writeRegistry() const
{
  size_t i = 0;

  for(auto it = m_registry.begin(); it != m_registry.end(); it++, i++) {
    setString(REGISTRY_GRP, ArrayKey(PACK_KEY, i), it->first);
    setString(REGISTRY_GRP, ArrayKey(VER_KEY, i), it->second);
  }
}
