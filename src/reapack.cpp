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

#include "reapack.hpp"

#include "config.hpp"
#include "manager.hpp"
#include "progress.hpp"
#include "report.hpp"
#include "transaction.hpp"

#include <reaper_plugin_functions.h>

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
  m_manager = Dialog::Create<Manager>(m_instance, m_mainWindow, this);
}

void ReaPack::cleanup()
{
  // for some reasons ~ReaPack() is called many times during startup
  // and two times during shutdown on osx... cleanup() is called only once
  m_config->write();
  delete m_config;

  Dialog::Destroy(m_progress);
  Dialog::Destroy(m_manager);
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
  const RemoteList *remotes = m_config->remotes();

  if(remotes->empty()) {
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
  static const char *title = "ReaPack: Import remote repository";

  string path(4096, 0);
  if(!GetUserFileNameForRead(&path[0], title, "ReaPackRemote"))
    return;

  Remote::ReadCode code;
  const Remote remote = Remote::fromFile(path, &code);

  switch(code) {
  case Remote::ReadFailure:
    ShowMessageBox(strerror(errno), title, 0);
    return;
  case Remote::InvalidName:
    ShowMessageBox("Invalid .ReaPackRemote file! (invalid name)", title, 0);
    return;
  case Remote::InvalidUrl:
    ShowMessageBox("Invalid .ReaPackRemote file! (invalid url)", title, 0);
    return;
  default:
    break;
  };

  RemoteList *remotes = m_config->remotes();

  const Remote existing = remotes->get(remote.name());

  if(!existing.isNull()) {
    if(existing.isProtected()) {
      ShowMessageBox(
        "This remote is protected and cannot be overwritten.", title, MB_OK);

      return;
    }
    else if(existing.url() == remote.url()) {
      ShowMessageBox(
        "This remote is already configured.\r\n"
        "Nothing to do!"
      , title, MB_OK);

      return;
    }

    const int button = ShowMessageBox(
      "This remote is already configured.\r\n"
      "Do you want to overwrite the current remote?"
      , title, MB_YESNO);

    if(button != IDYES)
      return;
  }

  remotes->add(remote);
  m_config->write();

  m_manager->refresh();

  Transaction *t = createTransaction();
  if(t)
    t->fetch(remote);
}

void ReaPack::manageRemotes()
{
  m_manager->refresh();

  if(m_manager->isVisible())
    m_manager->setFocus();
  else
    m_manager->show();
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

    m_progress->disable();

    if(m_transaction->packages().empty() && m_transaction->errors().empty())
      ShowMessageBox("Nothing to do!", "ReaPack", 0);
    else
      Dialog::Show<Report>(m_instance, m_mainWindow, m_transaction);

    m_progress->enable();
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
