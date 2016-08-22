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

#include "config.hpp"
#include "encoding.hpp"
#include "errors.hpp"
#include "filesystem.hpp"
#include "index.hpp"
#include "remote.hpp"
#include "task.hpp"

#include <boost/algorithm/string.hpp>

#include <reaper_plugin_functions.h>

using namespace std;

Transaction::Transaction(Config *config)
  : m_isCancelled(false), m_config(config),
    m_registry(Path::prefixRoot(Path::REGISTRY))
{
  // don't keep pre-install pushes (for conflict checks); released in runTasks
  m_registry.savepoint();

  m_downloadQueue.onAbort([=] {
    m_isCancelled = true;

    // clear the registration queue
    queue<HostTicket>().swap(m_regQueue);

    for(const TaskPtr &task : m_tasks)
      task->rollback();

    // some downloads may run for a few ms more
    if(m_downloadQueue.idle())
      finish();
  });

  // run tasks after fetching indexes
  m_downloadQueue.onDone([=] {
    if(m_tasks.empty())
      finish();
    else
      runTasks();
  });
}

void Transaction::synchronize(const Remote &remote,
  const boost::optional<bool> forceAutoInstall)
{
  if(m_syncedRemotes.count(remote.name()))
    return;

  m_syncedRemotes.insert(remote.name());

  InstallOpts opts = m_config->install;

  if(forceAutoInstall)
    opts.autoInstall = *forceAutoInstall;

  fetchIndex(remote, [=] {
    IndexPtr ri;

    try {
      ri = Index::load(remote.name());
    }
    catch(const reapack_error &e) {
      // index file is invalid (load error)
      addError(e.what(), remote.name());
      return;
    }

    for(const Package *pkg : ri->packages())
      synchronize(pkg, opts);
  });
}

void Transaction::synchronize(const Package *pkg, const InstallOpts &opts)
{
  const auto &regEntry = m_registry.getEntry(pkg);

  if(!regEntry && !opts.autoInstall)
    return;

  const Version *latest = pkg->lastVersion(opts.bleedingEdge, regEntry.version);

  // don't crash nor install a pre-release if autoInstall is on with
  // bleedingEdge mode off and there is no stable release
  if(!latest)
    return;

  if(regEntry.version == *latest) {
    if(allFilesExists(latest->files()))
      return; // latest version is really installed, nothing to do here!
  }
  else if(regEntry.pinned || *latest < regEntry.version)
    return;

  install(latest, regEntry);
}

void Transaction::fetchIndex(const Remote &remote, const function<void()> &cb)
{
  Download *dl = Index::fetch(remote, true, m_config->network);

  if(!dl) {
    // the index was last downloaded less than a few seconds ago
    cb();
    return;
  }

  m_downloadQueue.push(dl);
  dl->onFinish([=] {
    if(saveFile(dl, Index::pathFor(dl->name())))
      cb();
  });
}

void Transaction::install(const Version *ver)
{
  install(ver, m_registry.getEntry(ver->package()));
}

void Transaction::install(const Version *ver,
  const Registry::Entry &regEntry)
{
  InstallTicket::Type type;

  if(regEntry && regEntry.version < *ver)
    type = InstallTicket::Upgrade;
  else
    type = InstallTicket::Install;

  // get current files before overwriting the entry
  const auto &currentFiles = m_registry.getFiles(regEntry);

  // prevent file conflicts (don't worry, the registry push is reverted in runTasks)
  try {
    vector<Path> conflicts;
    m_registry.push(ver, &conflicts);

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
  IndexPtr ri = ver->package()->category()->index()->shared_from_this();
  if(!m_indexes.count(ri))
    m_indexes.insert(ri);

  auto task = make_shared<InstallTask>(ver, currentFiles, this);

  task->onCommit([=] {
    m_receipt.addTicket({type, ver, regEntry});

    for(const Registry::File &file : task->removedFiles()) {
      m_receipt.addRemoval(file.path);

      if(file.main)
        m_regQueue.push({false, regEntry, file});
    }

    const Registry::Entry newEntry = m_registry.push(ver);

    if(newEntry.type == Package::ExtensionType)
      m_receipt.setRestartNeeded(true);

    registerInHost(true, newEntry);
  });

  addTask(task);
}

void Transaction::registerAll(const Remote &remote)
{
  const vector<Registry::Entry> &entries = m_registry.getEntries(remote.name());

  for(const auto &entry : entries)
    registerInHost(remote.isEnabled(), entry);

  if(!remote.isEnabled())
    inhibit(remote);
}

void Transaction::setPinned(const Package *pkg, const bool pinned)
{
  // pkg may or may not be installed yet at this point,
  // waiting for the install task to be commited before querying the registry

  auto task = make_shared<DummyTask>(this);

  task->onCommit([=] {
    const Registry::Entry &entry = m_registry.getEntry(pkg);
    if(entry)
      m_registry.setPinned(entry, pinned);
  });

  addTask(task);
}

void Transaction::setPinned(const Registry::Entry &entry, const bool pinned)
{
  auto task = make_shared<DummyTask>(this);
  task->onCommit(bind(&Registry::setPinned, m_registry, entry, pinned));

  addTask(task);
}

void Transaction::uninstall(const Remote &remote)
{
  inhibit(remote);

  const Path &indexPath = Index::pathFor(remote.name());

  if(FS::exists(indexPath)) {
    if(!FS::remove(indexPath))
      addError(FS::lastError(), indexPath.join());
  }

  const vector<Registry::Entry> &entries = m_registry.getEntries(remote.name());

  if(entries.empty())
    return;

  for(const auto &entry : entries)
    uninstall(entry);
}

void Transaction::uninstall(const Registry::Entry &entry)
{
  vector<Path> files;

  for(const Registry::File &file : m_registry.getFiles(entry)) {
    if(FS::exists(file.path))
      files.push_back(file.path);
    if(file.main)
      m_regQueue.push({false, entry, file});
  }

  auto task = make_shared<RemoveTask>(files, this);

  task->onCommit([=] {
    m_registry.forget(entry);
    m_receipt.addRemovals(task->removedFiles());
  });

  addTask(task);
}

bool Transaction::saveFile(Download *dl, const Path &path)
{
  if(dl->state() != Download::Success) {
    addError(dl->contents(), dl->url());
    return false;
  }

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
    for(const TaskPtr &task : m_tasks)
      task->commit();

    m_registry.commit();

    registerQueued();
  }

  assert(m_downloadQueue.idle());
  assert(m_taskQueue.empty());
  assert(m_regQueue.empty());

  m_onFinish();
  m_cleanupHandler();
}

void Transaction::addError(const string &message, const string &title)
{
  m_receipt.addError({message, title});
}

bool Transaction::allFilesExists(const set<Path> &list) const
{
  for(const Path &path : list) {
    if(!FS::exists(path))
      return false;
  }

  return true;
}

void Transaction::addTask(const TaskPtr &task)
{
  m_tasks.push_back(task);
  m_taskQueue.push(task.get());
}

bool Transaction::runTasks()
{
  m_registry.restore();

  while(!m_taskQueue.empty()) {
    m_taskQueue.front()->start();
    m_taskQueue.pop();
  }

  // get ready for new tasks
  m_registry.savepoint();

  // return false if transaction is still in progress
  if(!m_downloadQueue.idle())
    return false;

  finish();
  return true;
}

void Transaction::registerInHost(const bool add, const Registry::Entry &entry)
{
  // don't actually do anything until commit() – which will calls registerQueued
  for(const Registry::File &file : m_registry.getMainFiles(entry))
    m_regQueue.push({add, entry, file});
}

void Transaction::registerQueued()
{
  while(!m_regQueue.empty()) {
    const HostTicket &reg = m_regQueue.front();

    // don't register in host if the remote got disabled meanwhile
    if(reg.add && m_inhibited.count(reg.entry.remote) > 0) {
      m_regQueue.pop();
      return;
    }

    switch(reg.file.type) {
    case Package::ScriptType:
      registerScript(reg);
      break;
    default:
      break;
    }

    m_regQueue.pop();
  }
}

void Transaction::registerScript(const HostTicket &reg)
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

  const string &fullPath = Path::prefixRoot(reg.file.path).join();
  const bool isLast = m_regQueue.size() == 1;

  if(!AddRemoveReaScript(reg.add, section, fullPath.c_str(), isLast) && reg.add)
    addError("This script could not be registered in REAPER.", reg.file.path.join());
}

void Transaction::inhibit(const Remote &remote)
{
  // prevents index post-download callbacks from being called
  // AND prevents files from this remote from being registered in REAPER
  // (UNregistering is not affected)

  const auto it = m_syncedRemotes.find(remote.name());
  if(it != m_syncedRemotes.end())
    m_syncedRemotes.erase(it);

  m_inhibited.insert(remote.name());
}
