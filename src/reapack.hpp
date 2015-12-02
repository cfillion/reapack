#ifndef REAPACK_REAPACK_HPP
#define REAPACK_REAPACK_HPP

#include <functional>
#include <map>

#include "config.hpp"
#include "database.hpp"
#include "download.hpp"

#include <reaper_plugin.h>

typedef std::function<void()> ActionCallback;

class ReaPack {
public:
  gaccel_register_t action;

  void init(REAPER_PLUGIN_HINSTANCE, reaper_plugin_info_t *);
  void cleanup();

  void setupAction(const char *name, const char *desc,
    gaccel_register_t *action, ActionCallback callback);
  bool execActions(const int id, const int);

  void synchronizeAll();
  void synchronize(const Repository &);
  void synchronize(DatabasePtr);
  void installPackage(PackagePtr);

private:
  std::map<int, ActionCallback> m_actions;

  Config m_config;
  DownloadQueue m_downloadQueue;

  REAPER_PLUGIN_HINSTANCE m_instance;
  reaper_plugin_info_t *m_rec;
  HWND m_mainHandle;
  Path m_resourcePath;
  Path m_dbPath;
};

#endif
