#include "config.hpp"

#include "path.hpp"

#include <swell/swell.h>

#include <reaper_plugin_functions.h>

using namespace std;

static const char *REMOTES_GROUP = "remotes";

static string RemoteNameKey(const int i)
{
  static const string key = "name";
  return key + to_string(i);
}

static string RemoteUrlKey(const int i)
{
  static const string key = "url";
  return key + to_string(i);
}

static const int URL_SIZE = 2083;

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
}

void Config::write() const
{
  writeRemotes();
}

void Config::readRemotes()
{
  int i = 0;

  do {
    char name[URL_SIZE];
    GetPrivateProfileString(REMOTES_GROUP, RemoteNameKey(i).c_str(),
      "", name, sizeof(name), m_path.c_str());

    char url[URL_SIZE];
    GetPrivateProfileString(REMOTES_GROUP, RemoteUrlKey(i).c_str(),
      "", url, sizeof(url), m_path.c_str());

    if(!strlen(name) || !strlen(url))
      break;

    m_remotes.push_back({name, url});
  } while(++i);
}

void Config::writeRemotes() const
{
  const int size = m_remotes.size();

  for(int i = 0; i < size; i++) {
    const Remote &remote = m_remotes[i];

    WritePrivateProfileString(REMOTES_GROUP, RemoteNameKey(i).c_str(),
      remote.name().c_str(), m_path.c_str());

    WritePrivateProfileString(REMOTES_GROUP, RemoteUrlKey(i).c_str(),
      remote.url().c_str(), m_path.c_str());
  }
}
