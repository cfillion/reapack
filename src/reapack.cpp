#include "reapack.hpp"

#include "transaction.hpp"

#include <reaper_plugin_functions.h>

#include <fstream>

using namespace std;

ReaPack::ReaPack()
  : m_transaction(0)
{
}

void ReaPack::init(REAPER_PLUGIN_HINSTANCE instance, reaper_plugin_info_t *rec)
{
  m_instance = instance;
  m_rec = rec;
  m_mainHandle = GetMainHwnd();
  m_resourcePath.append(GetResourcePath());

  // wtf? If m_config is a member object different instances will be used
  // in importRemote(), synchronize() and cleanup()
  m_config = new Config;
  m_config->read(m_resourcePath + "reapack.ini");
}

void ReaPack::cleanup()
{
  // for some reasons ~ReaPack() is called many times during startup
  // and two times during shutdown on osx... cleanup() is called only once
  m_config->write();
  delete m_config;
}

int ReaPack::setupAction(const char *name, const ActionCallback &callback)
{
  const int id = m_rec->Register("command_id", (void *)name);
  m_actions[id] = callback;

  return id;
}

int ReaPack::setupAction(const char *name, const char *desc,
  gaccel_register_t *action, const ActionCallback &callback)
{
  const int id = setupAction(name, callback);

  action->desc = desc;
  action->accel.cmd = id;

  m_rec->Register("gaccel", action);

  return id;
}

bool ReaPack::execActions(const int id, const int)
{
  if(!m_actions.count(id))
    return false;

  m_actions.at(id)();

  return true;
}

Transaction *ReaPack::createTransaction()
{
  if(m_transaction)
    return 0;

  m_transaction = new Transaction(m_config->registry(), m_resourcePath);

  m_transaction->onReady([=] {
    // TODO: display the package list with the changelogs
    m_transaction->run();
  });

  m_transaction->onFinish([=] {
    delete m_transaction;
    m_transaction = 0;

    m_config->write();
  });

  return m_transaction;
}

void ReaPack::synchronize()
{
  if(m_transaction)
    return;

  RemoteMap remotes = m_config->remotes();

  if(remotes.empty()) {
    ShowMessageBox("No remote repository configured, nothing to do!",
      "ReaPack", 0);

    return;
  }

  Transaction *t = createTransaction();
  if(t)
    m_transaction->fetch(remotes);
}

void ReaPack::importRemote()
{
  char path[4096];

  const char *title = "ReaPack: Import remote repository";
  if(!GetUserFileNameForRead(path, title, "ReaPackRemote"))
    return;

  ifstream file(path);
  if(!file.good()) {
    ShowMessageBox(strerror(errno), title, 0);
    return;
  }

  string name;
  file >> name;

  string url;
  file >> url;

  file.close();

  if(m_config->remotes().count({name})) {
    ShowMessageBox("This remote is already configured.", title, 0);
    return;
  }

  const Remote remote{name, url};
  m_config->addRemote(remote);
  m_config->write();

  Transaction *t = createTransaction();
  if(t)
    t->fetch(remote);
}
