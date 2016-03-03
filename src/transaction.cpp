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
#include "filesystem.hpp"
#include "index.hpp"
#include "remote.hpp"
#include "task.hpp"

#include <boost/algorithm/string.hpp>

#include <reaper_plugin_functions.h>

using namespace std;

Transaction::Transaction()
  : m_isCancelled(false), m_enableReport(false), m_needRestart(false)
{
  m_registry = new Registry(Path::prefixCache(Path::REGISTRY_FILE));

  m_downloadQueue.onDone([=](void *) {
    if(m_installQueue.empty())
      finish();
    else
      installQueued();
  });
}

Transaction::~Transaction()
{
  for(Task *task : m_tasks)
    delete task;

  for(const RemoteIndex *ri : m_remoteIndexes)
    delete ri;

  delete m_registry;
}

void Transaction::synchronize(const Remote &remote, const bool isUserAction)
{
  // show the report dialog even if no task are ran
  if(isUserAction)
    m_enableReport = true;

  fetchIndex(remote, [=] {
    const RemoteIndex *ri;

    try {
      ri = RemoteIndex::load(remote.name());
      m_remoteIndexes.push_back(ri);
    }
    catch(const reapack_error &e) {
      // index file is invalid (load error)
      addError(e.what(), remote.name());
      return;
    }

    // upgrade() will register all new packages to test for file conflicts.
    // Once this is done and all possible installations are queued,
    // we must restore the registry so the failed packages are not marked as
    // installed, and to keep the old file list intact (this is needed for
    // cleaning unused files)
    m_registry->savepoint();

    for(const Package *pkg : ri->packages())
      upgrade(pkg);

    m_registry->restore();
  });
}

void Transaction::fetchIndex(const Remote &remote, const IndexCallback &cb)
{
  // add the callback to the list, and start the download if it's the first one
  const std::string &name = remote.name();
  m_remotes.insert({name, cb});

  if(m_remotes.count(name) > 1)
    return;

  Download *dl = RemoteIndex::fetch(remote, true);

  if(!dl) {
    // the index was last downloaded less than a few seconds ago
    cb();
    finish();
    return;
  }

  m_downloadQueue.push(dl);
  dl->onFinish(bind(&Transaction::saveIndex, this, dl, name));
}

void Transaction::saveIndex(Download *dl, const string &name)
{
  if(!saveFile(dl, RemoteIndex::pathFor(name)))
    return;

  const auto end = m_remotes.upper_bound(name);

  for(auto it = m_remotes.lower_bound(name); it != end; it++)
    it->second();
}

void Transaction::upgrade(const Package *pkg)
{
  const Version *ver = pkg->lastVersion();
  auto queryRes = m_registry->query(pkg);

  if(queryRes.status == Registry::UpToDate) {
    if(allFilesExists(ver->files()))
      return; // latest version is really installed, nothing to do here!
    else
      queryRes.status = Registry::Uninstalled;
  }

  // prevent file conflicts – pushes to the registry will be reverted!
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
    // handle database error from Registry::push
    addError(e.what(), ver->fullName());
    return;
  }

  // all green! (pronounce with a japanese accent)
  m_installQueue.push({ver, queryRes});
}

void Transaction::installQueued()
{
  while(!m_installQueue.empty()) {
    installTicket(m_installQueue.front());
    m_installQueue.pop();
  }

  runTasks();
}

void Transaction::installTicket(const InstallTicket &ticket)
{
  const Version *ver = ticket.first;
  const Registry::QueryResult &queryRes = ticket.second;
  const set<Path> &currentFiles = m_registry->getFiles(queryRes.entry);

  InstallTask *task = new InstallTask(ver, currentFiles, this);

  task->onCommit([=] {
    if(queryRes.status == Registry::UpdateAvailable)
      m_updates.push_back(ticket);
    else
      m_new.push_back(ticket);

    const set<Path> &removedFiles = task->removedFiles();
    m_removals.insert(removedFiles.begin(), removedFiles.end());

    const Registry::Entry newEntry = m_registry->push(ver);

    if(newEntry.type == Package::ExtensionType)
      m_needRestart = true;

    registerInHost(true, newEntry);
  });

  addTask(task);
}

void Transaction::registerAll(const Remote &remote)
{
  const vector<Registry::Entry> &entries = m_registry->getEntries(remote);

  for(const auto &entry : entries)
    registerInHost(true, entry);
}

void Transaction::unregisterAll(const Remote &remote)
{
  const vector<Registry::Entry> &entries = m_registry->getEntries(remote);

  for(const auto &entry : entries)
    registerInHost(false, entry);

  inhibit(remote);
}

void Transaction::uninstall(const Remote &remote)
{
  inhibit(remote);
  remove(RemoteIndex::pathFor(remote.name()).join().c_str());

  const vector<Registry::Entry> &entries = m_registry->getEntries(remote);

  if(entries.empty())
    return;

  vector<Path> allFiles;

  for(const auto &entry : entries) {
    const set<Path> &files = m_registry->getFiles(entry);
    for(const Path &path : files) {
      if(file_exists(Path::prefixRoot(path).join().c_str()))
        allFiles.push_back(path);
    }

    registerInHost(false, entry);

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

  if(!FS::write(path, dl->contents())) {
    addError(FS::lastError(), path.join());
    return false;
  }

  return true;
}

void Transaction::finish()
{
  // called when the download queue is done, or if there is nothing to do

  if(!m_isCancelled) {
    for(Task *task : m_tasks)
      task->commit();

    m_registry->commit();

    registerQueued();
  }

  m_onFinish();
  m_onDestroy();
}

void Transaction::addError(const string &message, const string &title)
{
  m_errors.push_back({message, title});
  m_enableReport = true;
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

  m_enableReport = true;
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

void Transaction::registerInHost(const bool add, const Registry::Entry &entry)
{
  // don't actually do anything until commit() – which will calls registerQueued
  m_regQueue.push({add, entry, m_registry->getMainFile(entry)});
}

void Transaction::registerQueued()
{
  while(!m_regQueue.empty()) {
    const HostRegistration &reg = m_regQueue.front();

    // don't register in host if the remote got disabled meanwhile
    if(reg.add && m_remotes.count(reg.entry.remote) == 0) {
      m_regQueue.pop();
      return;
    }

    switch(reg.entry.type) {
    case Package::ScriptType:
      registerScript(reg);
      break;
    default:
      break;
    }

    m_regQueue.pop();
  }
}

void Transaction::registerScript(const HostRegistration &reg)
{
  enum Section { MainSection = 0, MidiEditorSection = 32060 };

  if(!AddRemoveReaScript)
    return; // do nothing if REAPER < v5.12

  Section section;
  string category = Path(reg.entry.category).first();
  boost::algorithm::to_lower(category);

  if(category == "midi editor")
    section = MidiEditorSection;
  else
    section = MainSection;

  const std::string &path = Path::prefixRoot(reg.file).join();
  const bool isLast = m_regQueue.size() == 1;

  if(!AddRemoveReaScript(reg.add, section, path.c_str(), isLast) && reg.add)
    addError("Script could not be registered in REAPER.", reg.file);
}

void Transaction::inhibit(const Remote &remote)
{
  // prevents index post-download callbacks from being called
  // AND prevents files from this remote from being registered in REAPER
  // (UNregistering is not affected)

  const auto it = m_remotes.find(remote.name());
  if(it != m_remotes.end())
    m_remotes.erase(it);
}
