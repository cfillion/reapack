/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2025  Christian Fillion
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

#include "action.hpp"
#include "api.hpp"
#include "browser.hpp"
#include "browser_entry.hpp"
#include "config.hpp"
#include "path.hpp"

#include <list>

#include <reaper_plugin.h>

class About;
class Browser;
class Manager;
class Progress;
class Remote;
class Transaction;

#define g_reapack (ReaPack::instance())

class ReaPack {
public:
  static ReaPack *instance() { return s_instance; }
  static Path resourcePath();

  ReaPack(REAPER_PLUGIN_HINSTANCE, HWND mainWindow);
  ~ReaPack();

  ActionList *actions() { return &m_actions; }

  void synchronizeAll();
  void uninstall(const Remote &);

  void uploadPackage();
  void importRemote();
  void manageRemotes();
  void aboutSelf();
  void about(const Remote &, bool focus = true);
  About *about(bool instantiate = true);
  Browser *browsePackages();
  void refreshManager();
  void refreshBrowser();
  bool requestProxy();

  void addSetRemote(const Remote &);
  Remote remote(const std::string &name) const;

  Transaction *setupTransaction();
  void commitConfig(bool refresh = true);
  Config *config() { return &m_config; }

private:
  static ReaPack *s_instance;

  void createDirectories();
  void registerSelf();
  void setupActions();
  void setupAPI();
  void teardownTransaction();

  REAPER_PLUGIN_HINSTANCE m_instance;
  HWND m_mainWindow;

  UseRootPath m_useRootPath;
  Config m_config;
  ActionList m_actions;
  std::list<APIReg> m_api;

  Transaction *m_tx;
  std::unique_ptr<About> m_about;
  std::unique_ptr<Browser> m_browser;
  std::unique_ptr<Manager> m_manager;
  std::unique_ptr<Progress> m_progress;
};

#endif
