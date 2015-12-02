#include "reapack.hpp"

#include "errors.hpp"

#include <fstream>

#include "reaper_plugin_functions.h"

using namespace std;

void ReaPack::init(REAPER_PLUGIN_HINSTANCE instance, reaper_plugin_info_t *rec)
{
  m_instance = instance;
  m_rec = rec;
  m_mainHandle = GetMainHwnd();
  m_resourcePath.append(GetResourcePath());

  m_config.read(m_resourcePath + "reapack.ini");

  m_dbPath = m_resourcePath + "ReaPack";

  RecursiveCreateDirectory(m_dbPath.cjoin(), 0);
}

void ReaPack::setupAction(const char *name, const char *desc,
  gaccel_register_t *action, ActionCallback callback)
{
  action->desc = desc;
  action->accel.cmd = m_rec->Register("command_id", (void *)name);

  m_rec->Register("gaccel", action);
  m_actions[action->accel.cmd] = callback;
}

bool ReaPack::execActions(const int id, const int)
{
  if(!m_actions.count(id))
    return false;

  m_actions.at(id)();

  return true;
}

void ReaPack::synchronizeAll()
{
  RepositoryList repos = m_config.repositories();

  if(repos.empty()) {
    ShowMessageBox("No repository configured, nothing to do!", "ReaPack", 0);
    return;
  }

  for(const Repository &repo : repos) {
    try {
      synchronize(repo);
    }
    catch(const reapack_error &e) {
      ShowMessageBox(e.what(), repo.name().c_str(), 0);
    }
  }
}

void ReaPack::synchronize(const Repository &repo)
{
  m_downloadQueue.push(repo.url(), [=](const int status, const string &contents) {
    if(status != 200)
      return;

    const Path path = m_dbPath + (repo.name() + ".xml");

    ofstream file(path.join());
    if(file.bad()) {
      ShowMessageBox(strerror(errno), repo.name().c_str(), 0);
      return;
    }

    file << contents;
    file.close();

    synchronize(Database::load(path.cjoin()));
  });
}

void ReaPack::synchronize(DatabasePtr database)
{
  if(database->packages().empty()) {
    ShowMessageBox("The package database is empty, nothing to do!",
      "ReaPack", 0);

    return;
  }

  for(PackagePtr pkg : database->packages()) {
    try {
      installPackage(pkg);
    }
    catch(const reapack_error &e) {
      ShowMessageBox(e.what(), "Package Error", 0);
    }
  }
}

void ReaPack::installPackage(PackagePtr pkg)
{
  const char *url = pkg->lastVersion()->source(0)->url().c_str();

  m_downloadQueue.push(url, [=](const int status, const string &contents) {
    if(status != 200) {
      ShowMessageBox(contents.c_str(), "Download failure (debug)", 0);
      return;
    }

    const Path path = m_resourcePath + pkg->targetLocation();
    RecursiveCreateDirectory(path.cdirname(), 0);

    ofstream file(path.join());
    if(file.bad()) {
      ShowMessageBox(strerror(errno), pkg->name().c_str(), 0);
      return;
    }

    file << contents;
    file.close();
  });
}
