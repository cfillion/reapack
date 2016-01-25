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

#include "errors.hpp"
#include "config.hpp"
#include "manager.hpp"
#include "progress.hpp"
#include "report.hpp"
#include "transaction.hpp"

#include <reaper_plugin_functions.h>

using namespace std;

ReaPack::ReaPack(REAPER_PLUGIN_HINSTANCE instance)
  : m_transaction(nullptr), m_instance(instance)
{
  m_mainWindow = GetMainHwnd();
  m_useRootPath = new UseRootPath(GetResourcePath());

  RecursiveCreateDirectory(Path::cacheDir().join().c_str(), 0);

  m_config = new Config;
  m_config->read(Path::configFile());

  m_progress = Dialog::Create<Progress>(m_instance, m_mainWindow);
  m_manager = Dialog::Create<Manager>(m_instance, m_mainWindow, this);

  Download::Init();
}

ReaPack::~ReaPack()
{
  m_config->write();
  delete m_config;

  Dialog::Destroy(m_progress);
  Dialog::Destroy(m_manager);

  Download::Cleanup();

  delete m_useRootPath;
}

int ReaPack::setupAction(const char *name, const ActionCallback &callback)
{
  const int id = plugin_register("command_id", (void *)name);
  m_actions[id] = callback;

  return id;
}

int ReaPack::setupAction(const char *name, const char *desc,
  gaccel_register_t *action, const ActionCallback &callback)
{
  const int id = setupAction(name, callback);

  action->desc = desc;
  action->accel.cmd = id;

  plugin_register("gaccel", action);

  return id;
}

bool ReaPack::execActions(const int id, const int)
{
  if(!m_actions.count(id))
    return false;

  m_actions.at(id)();

  return true;
}

void ReaPack::synchronizeAll()
{
  const vector<Remote> &remotes = m_config->remotes()->getEnabled();

  if(remotes.empty()) {
    ShowMessageBox("No remote repository enabled, nothing to do!",
      "ReaPack", 0);

    return;
  }

  // do nothing is a transation is already running
  Transaction *t = createTransaction();

  if(!t)
    return;

  for(const Remote &remote : remotes)
    t->synchronize(remote);
}

void ReaPack::synchronize(const Remote &remote)
{
  hitchhikeTransaction();

  // hitchhike currently running transactions
  m_transaction->synchronize(remote);
}

void ReaPack::enable(Remote remote)
{
  remote.setEnabled(true);
  m_config->remotes()->add(remote);

  if(!hitchhikeTransaction())
    return;

  m_transaction->registerAll(remote);
  m_transaction->synchronize(remote);
}

void ReaPack::disable(Remote remote)
{
  remote.setEnabled(false);
  m_config->remotes()->add(remote);

  if(!hitchhikeTransaction())
    return;

  m_transaction->unregisterAll(remote);
}

void ReaPack::uninstall(const Remote &remote)
{
  if(remote.isProtected())
    return;

  if(!hitchhikeTransaction())
    return;

  m_transaction->uninstall(remote);
  m_config->remotes()->remove(remote);
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

  const Remote &existing = remotes->get(remote.name());

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

  synchronize(remote);

  if(!m_transaction)
    return;

  m_transaction->onFinish([=] {
    if(m_transaction->isCancelled())
      return;

    remotes->add(remote);
    m_config->write();

    m_manager->refresh();
  });
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
  if(m_transaction) {
    m_progress->setFocus();
    return nullptr;
  }

  try {
    m_transaction = new Transaction;
  }
  catch(const reapack_error &e) {
    ShowMessageBox(e.what(), "ReaPack – Fatal Error", 0);
    return nullptr;
  }

  m_progress->setTransaction(m_transaction);

  m_transaction->onFinish([=] {
    if(m_transaction->isCancelled())
      return;

    m_progress->disable();
    m_manager->disable();

    if(m_transaction->taskCount() == 0 && m_transaction->errors().empty())
      ShowMessageBox("Nothing to do!", "ReaPack", 0);
    else
      Dialog::Show<Report>(m_instance, m_mainWindow, m_transaction);

    m_progress->enable();
    m_manager->enable();
  });

  m_transaction->onDestroy([=] {
    m_progress->setTransaction(nullptr);

    delete m_transaction;
    m_transaction = nullptr;
  });

  return m_transaction;
}

bool ReaPack::hitchhikeTransaction()
{
  if(m_transaction)
    return true;
  else
    return createTransaction() != nullptr;
}


void ReaPack::runTasks()
{
  if(m_transaction)
    m_transaction->runTasks();
}
