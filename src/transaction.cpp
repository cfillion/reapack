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

#include "transaction.hpp"

#include "encoding.hpp"
#include "errors.hpp"
#include "index.hpp"
#include "remote.hpp"
#include "task.hpp"

#include <fstream>

#include <reaper_plugin_functions.h>

using namespace std;

Transaction::Transaction()
  : m_isCancelled(false)
{
  m_registry = new Registry(Path::prefixCache("registry.db"));

  m_downloadQueue.onDone([=](void *) {
    if(m_installQueue.empty())
      finish();
    else
      install();
  });
}

Transaction::~Transaction()
{
  for(Task *task : m_tasks)
    delete task;

  for(RemoteIndex *ri : m_remoteIndexes)
    delete ri;

  delete m_registry;
}

void Transaction::synchronize(const Remote &remote)
{
  if(m_remotes.count(remote))
    return;

  Download *dl = new Download(remote.name(), remote.url());
  dl->onFinish(bind(&Transaction::upgradeAll, this, dl));

  m_downloadQueue.push(dl);
  m_remotes.insert(remote);
}

void Transaction::upgradeAll(Download *dl)
{
  const Path path = Path::prefixCache("remote_" + dl->name() + ".xml");

  if(!saveFile(dl, path))
    return;

  RemoteIndex *ri;

  try {
    ri = RemoteIndex::load(dl->name(), path.join().c_str());
    m_remoteIndexes.push_back(ri);
  }
  catch(const reapack_error &e) {
    addError(e.what(), dl->url());
    return;
  }

  for(Package *pkg : ri->packages())
    upgrade(pkg);
}

void Transaction::upgrade(Package *pkg)
{
  Version *ver = pkg->lastVersion();
  Registry::Entry entry = m_registry->query(pkg);

  try {
    vector<Path> conflicts;
    m_registry->push(ver, &conflicts);

    if(!conflicts.empty()) {
      for(const Path &path : conflicts) {
        addError("Conflict: " + path.join() +
          " is already owned by another package",
          ver->fullName());
      }

      return;
    }
  }
  catch(const reapack_error &e) {
    addError(e.what(), ver->fullName());
    return;
  }

  if(entry.status == Registry::UpToDate) {
    if(allFilesExists(ver->files()))
      return;
    else
      entry.status = Registry::Uninstalled;
  }

  m_installQueue.push({ver, entry});
}

void Transaction::install()
{
  while(!m_installQueue.empty()) {
    const PackageEntry entry = m_installQueue.front();
    m_installQueue.pop();

    Version *ver = entry.first;
    const Registry::Entry regEntry = entry.second;
    const set<Path> &currentFiles = m_registry->getFiles(regEntry);

    InstallTask *task = new InstallTask(ver, currentFiles, this);

    task->onCommit([=] {
      if(regEntry.status == Registry::UpdateAvailable)
        m_updates.push_back(entry);
      else
        m_new.push_back(entry);

      const set<Path> &removedFiles = task->removedFiles();
      m_removals.insert(removedFiles.begin(), removedFiles.end());
    });

    addTask(task);
  }

  runTasks();
}

void Transaction::registerAll(const Remote &)
{
}

void Transaction::unregisterAll(const Remote &)
{
}

void Transaction::uninstall(const Remote &remote)
{
  const vector<Registry::Entry> &entries = m_registry->queryAll(remote);

  if(entries.empty()) {
    cancel();
    return;
  }

  vector<Path> allFiles;

  for(const auto &entry : entries) {
    const set<Path> &files = m_registry->getFiles(entry);
    allFiles.insert(allFiles.end(), files.begin(), files.end());

    // forget the package even if some files cannot be removed
    m_registry->forget(entry);
  }

  RemoveTask *task = new RemoveTask(allFiles, this);

  task->onCommit([=] {
    const set<Path> &removedFiles = task->removedFiles();
    m_removals.insert(removedFiles.begin(), removedFiles.end());
  });

  addTask(task);
}

void Transaction::cancel()
{
  m_isCancelled = true;

  for(Task *task : m_tasks)
    task->rollback();

  if(m_downloadQueue.idle())
    finish();
  else
    m_downloadQueue.abort();
}

bool Transaction::saveFile(Download *dl, const Path &path)
{
  if(dl->state() != Download::Success) {
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
  // called when the download queue is done, or if there is nothing to do

  if(!m_isCancelled) {
    for(Task *task : m_tasks)
      task->commit();

    m_registry->commit();
  }

  m_onFinish();
  m_onDestroy();
}

void Transaction::addError(const string &message, const string &title)
{
  m_errors.push_back({message, title});
}

bool Transaction::allFilesExists(const set<Path> &list) const
{
  for(const Path &path : list) {
    if(!file_exists(Path::prefixRoot(path).join().c_str()))
      return false;
  }

  return true;
}

void Transaction::addTask(Task *task)
{
  m_tasks.push_back(task);
  m_taskQueue.push(task);
}

void Transaction::runTasks()
{
  while(!m_taskQueue.empty()) {
    m_taskQueue.front()->start();
    m_taskQueue.pop();
  }

  if(m_downloadQueue.idle())
    finish();
}
