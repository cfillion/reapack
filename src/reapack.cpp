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

#include "reapack.hpp"

#include "config.hpp"
#include "progress.hpp"
#include "report.hpp"
#include "transaction.hpp"

#include <reaper_plugin_functions.h>

#include <fstream>
#include <regex>

using namespace std;

ReaPack::ReaPack()
  : m_config(nullptr), m_transaction(nullptr), m_progress(nullptr)
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
  static const int PATH_SIZE = 4096;
  char path[PATH_SIZE] = {0};

  const char *title = "ReaPack: Import remote repository";
  if(!GetUserFileNameForRead(path, title, "ReaPackRemote"))
    return;

#ifdef _WIN32
  // fixes unicode path like "C:\Users\Test\Downloads\Новая папка" on Windows
  wchar_t wpath[PATH_SIZE];
  MultiByteToWideChar(CP_UTF8, 0, path, sizeof(path), wpath, PATH_SIZE);

  ifstream file(wpath);
#else
  ifstream file(path);
#endif

  if(!file) {
    ShowMessageBox(strerror(errno), title, 0);
    return;
  }

  string name;
  getline(file, name);

  string url;
  getline(file, url);

  file.close();

  static const regex namePattern("^[\\w\\s]+$");

  smatch nameMatch;
  regex_match(name, nameMatch, namePattern);

  if(nameMatch.empty() || url.empty()) {
    ShowMessageBox("Invalid .ReaPackRemote file!", title, 0);
    return;
  }

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
    return nullptr;

  m_transaction = new Transaction(m_config->registry(), m_resourcePath);

  m_progress->setTransaction(m_transaction);
  m_progress->show();

  m_transaction->onReady([=] {
    // TODO: display the package list with the changelogs
    m_transaction->run();
  });

  m_transaction->onFinish([=] {
    if(m_transaction->isCancelled())
      return;

    m_progress->setEnabled(false);

    if(m_transaction->packages().empty() && m_transaction->errors().empty())
      ShowMessageBox("Nothing to do!", "ReaPack", 0);
    else
      Dialog::Show<Report>(m_instance, m_mainWindow, m_transaction);

    m_progress->setEnabled(true);
    m_progress->hide();

    m_config->write();
  });

  m_transaction->onDestroy([=] {
    m_progress->setTransaction(nullptr);

    delete m_transaction;
    m_transaction = nullptr;
  });

  return m_transaction;
}
