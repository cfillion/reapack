#include "config.hpp"

#include "path.hpp"

#include <swell/swell.h>

#include <reaper_plugin_functions.h>

using namespace std;

static const char *REPOS_GROUP = "repositories";

static string RepoNameKey(const int i)
{
  static const string key = "name";
  return key + to_string(i);
}

static string RepoUrlKey(const int i)
{
  static const string key = "url";
  return key + to_string(i);
}

static const int URL_SIZE = 2083;

Config::Config()
{
}

void Config::fillDefaults()
{
  m_repositories.push_back({"ReaScripts",
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

  readRepositories();
}

void Config::write() const
{
  writeRepositories();
}

void Config::readRepositories()
{
  int i = 0;

  do {
    char name[URL_SIZE];
    GetPrivateProfileString(REPOS_GROUP, RepoNameKey(i).c_str(),
      "", name, sizeof(name), m_path.c_str());

    char url[URL_SIZE];
    GetPrivateProfileString(REPOS_GROUP, RepoUrlKey(i).c_str(),
      "", url, sizeof(url), m_path.c_str());

    if(!strlen(name) || !strlen(url))
      break;

    m_repositories.push_back({name, url});
  } while(++i);
}

void Config::writeRepositories() const
{
  const int size = m_repositories.size();

  for(int i = 0; i < size; i++) {
    const Repository &repo = m_repositories[i];

    WritePrivateProfileString(REPOS_GROUP, RepoNameKey(i).c_str(),
      repo.name().c_str(), m_path.c_str());

    WritePrivateProfileString(REPOS_GROUP, RepoUrlKey(i).c_str(),
      repo.url().c_str(), m_path.c_str());
  }
}
