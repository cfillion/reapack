#include "reapack.hpp"

#include "transaction.hpp"

#include <reaper_plugin_functions.h>

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

  m_config.read(m_resourcePath + "reapack.ini");
}

void ReaPack::cleanup()
{
  // for some reasons ~ReaPack() is called many times during startup
  // and two times during shutdown on osx... cleanup() is called only once
  m_config.write();
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
  if(m_transaction)
    return;

  RemoteList remotes = m_config.remotes();

  if(remotes.empty()) {
    ShowMessageBox("No remote repository configured, nothing to do!",
      "ReaPack", 0);

    return;
  }

  m_transaction = new Transaction(m_resourcePath);

  m_transaction->onReady([=] {
    // TODO: display the package list with the changelogs
    m_transaction->run();
  });

  m_transaction->onFinish([=] {
    delete m_transaction;
    m_transaction = 0;
  });

  m_transaction->fetch(remotes);
}
