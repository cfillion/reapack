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

Transaction::Transaction(const InstallOpts &opts)
  : m_isCancelled(false), m_opts(opts)
{
  m_registry = new Registry(Path::prefixRoot(Path::REGISTRY));

  // don't keep pre-install pushes (for conflict checks); released in runTasks
  m_registry->savepoint();

  m_downloadQueue.onAbort([=] {
    m_isCancelled = true;

    for(Task *task : m_tasks)
      task->rollback();

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

Transaction::~Transaction()
{
  for(Task *task : m_tasks)
    delete task;

  delete m_registry;
}

void Transaction::synchronize(const Remote &remote)
{
  // show the report dialog or "nothing to do" even if no task are ran
  m_receipt.setEnabled(true);

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
      synchronize(pkg);
  });
}

void Transaction::synchronize(const Package *pkg)
{
  const auto &regEntry = m_registry->getEntry(pkg);

  if(!regEntry && !m_opts.autoInstall)
    return;

  const Version *latest = pkg->lastVersion(m_opts.bleedingEdge, regEntry.version);

  // don't crash nor install a pre-release if autoInstall is on,
  // bleedingEdge is off and there is no stable release
  if(!latest)
    return;

  if(regEntry.version == *latest) {
    if(allFilesExists(latest->files())) {
      installDependencies(latest); // are dependency installed too?
      return; // latest version is really installed, nothing to do here!
    }
  }
  else if(regEntry.pinned || *latest < regEntry.version)
    return;

  install(latest, regEntry);
}

void Transaction::fetchIndex(const Remote &remote, const IndexCallback &cb)
{
  // add the callback to the list, and start the download if it's the first one
  const string &name = remote.name();
  m_remotes.insert({name, cb});

  if(m_remotes.count(name) > 1)
    return;

  Download *dl = Index::fetch(remote, true);

  if(!dl) {
    // the index was last downloaded less than a few seconds ago
    cb();
    return;
  }

  m_downloadQueue.push(dl);
  dl->onFinish(bind(&Transaction::saveIndex, this, dl, name));
}

void Transaction::saveIndex(Download *dl, const string &name)
{
  if(!saveFile(dl, Index::pathFor(name)))
    return;

  const auto end = m_remotes.upper_bound(name);

  for(auto it = m_remotes.lower_bound(name); it != end; it++)
    it->second();
}

void Transaction::install(const Version *ver)
{
  install(ver, m_registry->getEntry(ver->package()));
}

bool Transaction::install(const Version *ver,
  const Registry::Entry &regEntry)
{
  // do nothing if the package is already queued for installation
  if(m_installs.count(ver->package()))
    return true;

  m_receipt.setEnabled(true);

  InstallTicket::Type type;

  if(regEntry && regEntry.version < *ver)
    type = InstallTicket::Upgrade;
  else
    type = InstallTicket::Install;

  // prevent file conflicts (don't worry, the registry push is reverted in runTasks)
  try {
    vector<Path> conflicts;
    m_registry->push(ver, &conflicts);

    if(!conflicts.empty()) {
      for(const Path &path : conflicts) {
        addError("Conflict: " + path.join() +
          " is already owned by another package",
          ver->fullName());
      }

      return false;
    }
  }
  catch(const reapack_error &e) {
    // handle database error from Registry::push
    addError(e.what(), ver->fullName());
    return false;
  }

  // don't try to reinstall this pacakge later as it would cause the install
  // task to try renaming .new file twice (and failing the second time)
  //
  // also prevents circular dependencies from causing an infinite loop
  m_installs.insert(ver->package());

  if(!installDependencies(ver)) {
    m_installs.erase(ver->package());
    return false;
  }

  // all green! (pronounce with a japanese accent)
  IndexPtr ri = ver->package()->category()->index()->shared_from_this();
  if(!m_indexes.count(ri))
    m_indexes.insert(ri);

  const set<Path> &currentFiles = m_registry->getFiles(regEntry);

  InstallTask *task = new InstallTask(ver, currentFiles, this);

  task->onCommit([=] {
    m_receipt.addTicket({type, ver, regEntry});
    m_receipt.addRemovals(task->removedFiles());

    const Registry::Entry newEntry = m_registry->push(ver);

    if(newEntry.type == Package::ExtensionType)
      m_receipt.setRestartNeeded(true);

    registerInHost(true, newEntry);
  });

  addTask(task);
  m_receipt.setEnabled(true);
  return true;
}

bool Transaction::installDependencies(const Version *ver)
{
  for(const Dependency &dep : ver->dependencies()) {
    printf("resolving %s\n", dep.path.c_str());
    const Index *index = ver->package()->category()->index();
    const Package *depPkg = index->findPackage(dep.path);

    if(!depPkg) {
      addError("Dependency not found: " + dep.path, ver->fullName());
      return false;
    }

    const Registry::Entry &regEntry = m_registry->getEntry(depPkg);
    const Version *depVer = nullptr;

    if(regEntry) {
      // dependency is installed, check for missing files if possible
      // TODO: minimum version. what happens if older version is pinned?
      depVer = depPkg->findVersion(regEntry.version);
      if(!depVer || allFilesExists(depVer->files()))
        continue;
    }
    else {
      // select the best available version
      depVer = depPkg->lastVersion(m_opts.bleedingEdge); // TODO: minimum version
      if(!depVer)
        depVer = depPkg->lastVersion(true);
    }

    if(!install(depVer, regEntry))
      return false;
  }

  return true;
}

void Transaction::registerAll(const Remote &remote)
{
  const vector<Registry::Entry> &entries = m_registry->getEntries(remote.name());

  for(const auto &entry : entries)
    registerInHost(remote.isEnabled(), entry);

  if(!remote.isEnabled())
    inhibit(remote);
}

void Transaction::setPinned(const Package *pkg, const bool pinned)
{
  // pkg may or may not be installed yet at this point,
  // waiting for the install task to be commited before querying the registry

  DummyTask *task = new DummyTask(this);
  task->onCommit([=] {
    const Registry::Entry &entry = m_registry->getEntry(pkg);
    if(entry)
      m_registry->setPinned(entry, pinned);
  });

  addTask(task);
}

void Transaction::setPinned(const Registry::Entry &entry, const bool pinned)
{
  DummyTask *task = new DummyTask(this);
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

  const vector<Registry::Entry> &entries = m_registry->getEntries(remote.name());

  if(entries.empty())
    return;

  for(const auto &entry : entries)
    uninstall(entry);
}

void Transaction::uninstall(const Registry::Entry &entry)
{
  vector<Path> files;

  for(const Path &path : m_registry->getFiles(entry)) {
    if(FS::exists(path))
      files.push_back(path);
  }

  registerInHost(false, entry);

  RemoveTask *task = new RemoveTask(files, this);
  task->onCommit([=] {
    m_registry->forget(entry);
    m_receipt.addRemovals(task->removedFiles());
  });

  addTask(task);
  m_receipt.setEnabled(true);
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
    for(Task *task : m_tasks)
      task->commit();

    m_registry->commit();

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
  m_receipt.setEnabled(true);
}

bool Transaction::allFilesExists(const set<Path> &list) const
{
  for(const Path &path : list) {
    if(!FS::exists(path))
      return false;
  }

  return true;
}

void Transaction::addTask(Task *task)
{
  m_tasks.push_back(task);
  m_taskQueue.push(task);
}

bool Transaction::runTasks()
{
  m_registry->restore();

  while(!m_taskQueue.empty()) {
    m_taskQueue.front()->start();
    m_taskQueue.pop();
  }

  m_registry->savepoint(); // get ready for new tasks

  if(!m_downloadQueue.idle())
    return false;

  finish();
  return true;
}

void Transaction::registerInHost(const bool add, const Registry::Entry &entry)
{
  // don't actually do anything until commit() – which will calls registerQueued
  const string &mainFile = m_registry->getMainFile(entry);
  if(!mainFile.empty())
    m_regQueue.push({add, entry, mainFile});
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

  const string &fullPath = Path::prefixRoot(reg.file).join();
  const bool isLast = m_regQueue.size() == 1;

  if(!AddRemoveReaScript(reg.add, section, fullPath.c_str(), isLast) && reg.add)
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

  m_inhibited.insert(remote.name());
}
