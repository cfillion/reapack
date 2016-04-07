/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2016  Christian Fillion
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef REAPACK_REAPACK_HPP
#define REAPACK_REAPACK_HPP

#include "path.hpp"
#include "registry.hpp"

#include <functional>
#include <map>
#include <memory>
#include <vector>

#include <reaper_plugin.h>

class Browser;
class Config;
class DownloadQueue;
class Import;
class Index;
class Manager;
class Progress;
class Remote;
class Transaction;

typedef std::shared_ptr<const Index> IndexPtr;

class ReaPack {
public:
  typedef std::function<void ()> ActionCallback;
  typedef std::function<void (IndexPtr)> IndexCallback;
  typedef std::function<void (const std::vector<IndexPtr> &)> IndexesCallback;

  static const std::string VERSION;
  static const std::string BUILDTIME;

  gaccel_register_t syncAction;
  gaccel_register_t browseAction;
  gaccel_register_t importAction;
  gaccel_register_t configAction;

  ReaPack(REAPER_PLUGIN_HINSTANCE);
  ~ReaPack();

  int setupAction(const char *name, const ActionCallback &);
  int setupAction(const char *name, const char *desc,
    gaccel_register_t *action, const ActionCallback &);
  bool execActions(int id, int);

  void synchronizeAll();
  void setRemoteEnabled(const Remote &, bool enable);
  void enable(const Remote &r) { setRemoteEnabled(r, true); }
  void disable(const Remote &r) { setRemoteEnabled(r, false); }
  void install(const Version *);
  void uninstall(const Remote &);
  void uninstall(const Registry::Entry &);
  void importRemote();
  void import(const Remote &);
  void manageRemotes();
  void aboutSelf();
  void about(const std::string &, HWND parent);
  void about(const Remote &, HWND parent);
  void browsePackages();
  void fetchIndex(const Remote &remote, const IndexCallback &, HWND);
  void fetchIndexes(const std::vector<Remote> &,
    const IndexesCallback &, HWND = nullptr);

  void runTasks();

  Config *config() const { return m_config; }

private:
  Transaction *createTransaction();
  bool hitchhikeTransaction();
  void registerSelf();
  void doFetchIndex(const Remote &remote, DownloadQueue *, HWND);
  IndexPtr loadIndex(const Remote &remote, HWND);

  std::map<int, ActionCallback> m_actions;

  Config *m_config;
  Transaction *m_transaction;
  Progress *m_progress;
  Browser *m_browser;
  Import *m_import;
  Manager *m_manager;

  REAPER_PLUGIN_HINSTANCE m_instance;
  HWND m_mainWindow;
  UseRootPath *m_useRootPath;
};

#endif
