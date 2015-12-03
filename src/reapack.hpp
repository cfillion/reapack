#ifndef REAPACK_REAPACK_HPP
#define REAPACK_REAPACK_HPP

#include <functional>
#include <map>

#include "config.hpp"
#include "path.hpp"

#include <reaper_plugin.h>

typedef std::function<void()> ActionCallback;

class Transaction;

class ReaPack {
public:
  gaccel_register_t action;

  ReaPack();

  void init(REAPER_PLUGIN_HINSTANCE, reaper_plugin_info_t *);
  void cleanup();

  void setupAction(const char *name, const char *desc,
    gaccel_register_t *action, ActionCallback callback);
  bool execActions(const int id, const int);

  void synchronize();

private:
  std::map<int, ActionCallback> m_actions;

  Config m_config;
  Transaction *m_transaction;

  REAPER_PLUGIN_HINSTANCE m_instance;
  reaper_plugin_info_t *m_rec;
  HWND m_mainHandle;
  Path m_resourcePath;
};

#endif
