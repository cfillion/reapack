#ifndef REAPACK_REAPACK_HPP
#define REAPACK_REAPACK_HPP

#include <functional>
#include <map>

#include "reaper_plugin.h"

typedef std::function<void()> ActionCallback;

class ReaPack {
public:
  gaccel_register_t action;

  void init(REAPER_PLUGIN_HINSTANCE, reaper_plugin_info_t *);

  void setupAction(const char *name, const char *desc,
    gaccel_register_t *action, ActionCallback callback);
  bool execActions(const int id, const int);

  void toggleWindow();

private:
  std::map<int, ActionCallback> m_actions;

  REAPER_PLUGIN_HINSTANCE m_instance;
  reaper_plugin_info_t *m_rec;
  HWND m_mainHandle;
};

#endif
