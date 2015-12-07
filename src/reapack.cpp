#include "reapack.hpp"

#include "config.hpp"
#include "progress.hpp"
#include "report.hpp"
#include "transaction.hpp"

#include <reaper_plugin_functions.h>

#include <fstream>

using namespace std;

ReaPack::ReaPack()
  : m_config(0), m_transaction(0), m_progress(0)
{
}

void ReaPack::init(REAPER_PLUGIN_HINSTANCE instance, reaper_plugin_info_t *rec)
{
  m_instance = instance;
  m_rec = rec;
  m_mainWindow = GetMainHwnd();
  m_resourcePath.append(GetResourcePath());

  m_config = new Config;
  m_config->read(m_resourcePath + "reapack.ini");

  m_progress = Dialog::Create<Progress>(m_instance, m_mainWindow);
}

void ReaPack::cleanup()
{
  // for some reasons ~ReaPack() is called many times during startup
  // and two times during shutdown on osx... cleanup() is called only once
  m_config->write();
  delete m_config;

  Dialog::Destroy(m_progress);
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

void ReaPack::synchronize()
{
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
  getline(file, name);

  string url;
  getline(file, url);

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
    m_progress->setEnabled(false);

    if(m_transaction->packages().empty())
      ShowMessageBox("Nothing to do!", "ReaPack", 0);
    else
      Dialog::Show<Report>(m_instance, m_mainWindow, m_transaction);

    m_progress->setEnabled(true);
    m_progress->setTransaction(0);
    m_progress->hide();

    delete m_transaction;
    m_transaction = 0;

    m_config->write();
  });

  m_progress->setTransaction(m_transaction);
  m_progress->show();

  return m_transaction;
}
