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
  m_resourcePath = GetResourcePath();
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

void ReaPack::synchronize()
{
  try {
    m_database = Database::load("/Users/cfillion/Programs/reapack/reapack.xml");

    for(PackagePtr pkg : m_database->packages()) {
      installPackage(pkg);
    }
  }
  catch(const reapack_error &e) {
    ShowMessageBox(e.what(), "Database Error", 0);
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

    InstallLocation loc = pkg->targetLocation();
    loc.prependDir(m_resourcePath);

    RecursiveCreateDirectory(loc.directory().c_str(), 0);

    ofstream file(loc.fullPath());
    if(file.bad()) {
      ShowMessageBox(strerror(errno), pkg->name().c_str(), 0);
      return;
    }

    file << contents;
    file.close();
  });
}
