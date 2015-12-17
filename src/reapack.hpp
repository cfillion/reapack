/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015  Christian Fillion
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

#include <functional>
#include <map>

#include "path.hpp"

#include <reaper_plugin.h>

typedef std::function<void()> ActionCallback;

class Config;
class Transaction;
class Progress;

class ReaPack {
public:
  gaccel_register_t syncAction;

  ReaPack();

  void init(REAPER_PLUGIN_HINSTANCE, reaper_plugin_info_t *);
  void cleanup();

  int setupAction(const char *name, const ActionCallback &);
  int setupAction(const char *name, const char *desc,
    gaccel_register_t *action, const ActionCallback &);
  bool execActions(const int id, const int);

  void synchronize();
  void importRemote();

private:
  Transaction *createTransaction();

  std::map<int, ActionCallback> m_actions;

  Config *m_config;
  Transaction *m_transaction;
  Progress *m_progress;

  REAPER_PLUGIN_HINSTANCE m_instance;
  reaper_plugin_info_t *m_rec;
  HWND m_mainWindow;
  Path m_resourcePath;
};

#endif
