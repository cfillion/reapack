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

#include "transaction.hpp"

#include "encoding.hpp"
#include "errors.hpp"
#include "task.hpp"

#include <fstream>

#include <reaper_plugin_functions.h>

using namespace std;

Transaction::Transaction(Registry *reg, const Path &root)
  : m_registry(reg), m_root(root), m_isCancelled(false), m_hasConflicts(false)
{
  m_dbPath = m_root + "ReaPack";

  RecursiveCreateDirectory(m_dbPath.join().c_str(), 0);
}

Transaction::~Transaction()
{
  for(Task *task : m_tasks)
    delete task;

  for(Database *db : m_databases)
    delete db;
}

void Transaction::fetch(const RemoteMap &remotes)
{
  for(const Remote &remote : remotes)
    fetch(remote);
}

void Transaction::fetch(const Remote &remote)
{
  Download *dl = new Download(remote.first, remote.second);
  dl->onFinish(bind(&Transaction::saveDatabase, this, dl));

  m_queue.push(dl);

  // execute prepare after the download is deleted, in case finish is called
  // the queue will also not contain the download anymore
  dl->onFinish(bind(&Transaction::prepare, this));
}

void Transaction::saveDatabase(Download *dl)
{
  const Path path = m_dbPath + ("remote_" + dl->name() + ".xml");

  if(!saveFile(dl, path))
    return;

  try {
    Database *db = Database::load(dl->name(), path.join().c_str());

    m_databases.push_back(db);
  }
  catch(const reapack_error &e) {
    addError(e.what(), dl->url());
  }
}

void Transaction::prepare()
{
  if(!m_queue.idle())
    return;

  for(Database *db : m_databases) {
    for(Package *pkg : db->packages()) {
      Registry::QueryResult entry = m_registry->query(pkg);

      set<Path> files = pkg->lastVersion()->files();
      registerFiles(files);

      if(entry.status == Registry::UpToDate) {
        if(allFilesExists(files))
          continue;
        else
          entry.status = Registry::Uninstalled;
      }

      m_packages.push_back({pkg, entry});
    }
  }

  if(m_packages.empty() || m_hasConflicts)
    finish();
  else
    m_onReady();
}

void Transaction::run()
{
  for(const PackageEntry &entry : m_packages) {
    Package *pkg = entry.first;
    const Registry::QueryResult regEntry = entry.second;

    Task *task = new Task(this);

    try {
      task->install(entry.first->lastVersion());
      task->onCommit([=] {
        if(regEntry.status == Registry::UpdateAvailable)
          m_updates.push_back(entry);
        else
          m_new.push_back(entry);

        m_registry->push(pkg);
      });
      task->onFinish(bind(&Transaction::finish, this));

      m_tasks.push_back(task);
    }
    catch(const reapack_error &e) {
      addError(e.what(), pkg->fullName());
      delete task;
    }
  }
}

void Transaction::cancel()
{
  m_isCancelled = true;

  for(Task *task : m_tasks)
    task->cancel();

  m_queue.abort();
}

bool Transaction::saveFile(Download *dl, const Path &path)
{
  if(dl->status() != 200) {
    addError(dl->contents(), dl->url());
    return false;
  }

  RecursiveCreateDirectory(path.dirname().c_str(), 0);

  const string strPath = path.join();
  ofstream file(make_autostring(strPath), ios_base::binary);

  if(!file) {
    addError(strerror(errno), strPath);
    return false;
  }

  file << dl->contents();
  file.close();

  return true;
}

void Transaction::finish()
{
  if(!m_queue.idle())
    return;

  if(!m_isCancelled) {
    for(Task *task : m_tasks)
      task->commit();
  }

  m_onFinish();
  m_onDestroy();
}

void Transaction::addError(const string &message, const string &title)
{
  m_errors.push_back({message, title});
}

Path Transaction::prefixPath(const Path &input) const
{
  return m_root + input;
}

bool Transaction::allFilesExists(const set<Path> &list) const
{
  for(const Path &path : list) {
    if(!file_exists(prefixPath(path).join().c_str()))
      return false;
  }

  return true;
}

void Transaction::registerFiles(const set<Path> &list)
{
  for(const Path &path : list) {
    if(!m_files.count(path))
      continue;

    addError("Conflict: This file is owned by more than one package",
      path.join());

    m_hasConflicts = true;
  }

  m_files.insert(list.begin(), list.end());
}
