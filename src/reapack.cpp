#include "reapack.hpp"

#include "errors.hpp"
#include "database.hpp"

#include "reaper_plugin_functions.h"

void ReaPack::init(REAPER_PLUGIN_HINSTANCE instance, reaper_plugin_info_t *rec)
{
  m_instance = instance;
  m_rec = rec;
  m_mainHandle = GetMainHwnd();
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

void ReaPack::toggleBrowser()
{
  try {
    Database::load("/Users/cfillion/Programs/reapack/reapack.xml");
    ShowMessageBox("Hello World!", "Test", 0);
  }
  catch(const reapack_error &e) {
    ShowMessageBox(e.what(), "Database Error", 0);
  }
}
