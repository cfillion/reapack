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

#include "transaction.hpp"

#include "config.hpp"
#include "download.hpp"
#include "errors.hpp"
#include "filesystem.hpp"
#include "index.hpp"
#include "reapack.hpp"
#include "remote.hpp"
#include "task.hpp"

#include <cassert>

#include <reaper_plugin_functions.h>

Transaction::Transaction()
  : m_isCancelled(false), m_registry(Path::REGISTRY.prependRoot())
{
  m_threadPool.onPush >> [this] (ThreadTask *task) {
    task->onFinishAsync >> [=] {
      if(task->state() == ThreadTask::Failure)
        m_receipt.addError(task->error());
    };
  };

  m_threadPool.onAbort >> [this] {
    m_isCancelled = true;
    std::queue<HostTicket>().swap(m_regQueue);
  };

  // run the next task queue when the current one is done
  m_threadPool.onDone >> std::bind(&Transaction::runTasks, this);
}

void Transaction::synchronize(const Remote &remote,
  const std::optional<bool> &forceAutoInstall)
{
  if(m_syncedRemotes.count(remote.name()))
    return;

  m_syncedRemotes.insert(remote.name());

  InstallOpts opts = g_reapack->config()->install;
  opts.autoInstall = remote.autoInstall(forceAutoInstall.value_or(opts.autoInstall));

  m_nextQueue.push(std::make_shared<SynchronizeTask>(remote, true, true, opts, this));
}

void Transaction::fetchIndexes(const std::vector<Remote> &remotes, const bool stale)
{
  for(const Remote &remote : remotes)
    m_nextQueue.push(std::make_shared<SynchronizeTask>(remote, stale, false, InstallOpts{}, this));
}

std::vector<IndexPtr> Transaction::getIndexes(const std::vector<Remote> &remotes) const
{
  std::vector<IndexPtr> indexes;

  for(const Remote &remote : remotes) {
    const auto &it = m_indexes.find(remote.name());
    if(it != m_indexes.end())
      indexes.push_back(it->second);
  }

  return indexes;
}

IndexPtr Transaction::loadIndex(const Remote &remote)
{
  const auto &it = m_indexes.find(remote.name());
  if(it != m_indexes.end())
    return it->second;

  try {
    const IndexPtr &ri = Index::load(remote.name());
    m_indexes[remote.name()] = ri;
    return ri;
  }
  catch(const reapack_error &e) {
    m_receipt.addError({
      String::format("Could not load repository: %s", e.what()), remote.name()});
    return nullptr;
  }
}

void Transaction::install(const Version *ver, const int flags,
  const ArchiveReaderPtr &reader)
{
  install(ver, m_registry.getEntry(ver->package()), flags, reader);
}

void Transaction::install(const Version *ver, const Registry::Entry &oldEntry,
  const int flags, const ArchiveReaderPtr &reader)
{
  m_nextQueue.push(std::make_shared<InstallTask>(ver, flags, oldEntry, reader, this));
}

void Transaction::setFlags(const Registry::Entry &entry, const int flags)
{
  m_nextQueue.push(std::make_shared<FlagsTask>(entry, flags, this));
}

void Transaction::uninstall(const Remote &remote)
{
  inhibit(remote);

  const Path &indexPath = Index::pathFor(remote.name());

  if(FS::exists(indexPath)) {
    if(!FS::remove(indexPath))
      m_receipt.addError({FS::lastError(), indexPath.join()});
  }

  for(const auto &entry : m_registry.getEntries(remote.name()))
    uninstall(entry);
}

void Transaction::uninstall(const Registry::Entry &entry)
{
  m_nextQueue.push(std::make_shared<UninstallTask>(entry, this));
}

void Transaction::exportArchive(const std::string &path)
{
  m_nextQueue.push(std::make_shared<ExportTask>(path, this));
}

bool Transaction::runTasks()
{
  do {
    if(!m_nextQueue.empty()) {
      m_taskQueues.push(m_nextQueue);
      TaskQueue().swap(m_nextQueue);
    }

    if(!commitTasks())
      return false; // we're downloading indexes for synchronization
    else if(m_isCancelled) {
      finish();
      return true;
    }

    promptObsolete();

    while(!m_taskQueues.empty()) {
      runQueue(m_taskQueues.front());
      m_taskQueues.pop();

      if(!commitTasks())
        return false; // if the tasks didn't finish immediately (downloading)
    }
  } while(!m_nextQueue.empty()); // restart if a task's commit() added new tasks

  finish(); // we're done!

  return true;
}

void Transaction::runQueue(TaskQueue &queue)
{
  m_registry.savepoint();

  while(!queue.empty()) {
    const TaskPtr &task = queue.top();

    if(task->start())
      m_runningTasks.push(task);

    queue.pop();
  }

  m_registry.restore();
}

bool Transaction::commitTasks()
{
  // wait until all running tasks are ready
  if(!m_threadPool.idle())
    return false;

  // finish current tasks
  while(!m_runningTasks.empty()) {
    if(m_isCancelled)
      m_runningTasks.front()->rollback();
    else
      m_runningTasks.front()->commit();

    m_runningTasks.pop();
  }

  return true;
}

void Transaction::finish()
{
  m_registry.commit();
  registerQueued();

  onFinish();
  m_cleanupHandler();
}

void Transaction::registerAll(const bool add, const Registry::Entry &entry)
{
  // don't actually do anything until commit() – which will calls registerQueued
  for(const Registry::File &file : m_registry.getMainFiles(entry))
    registerFile({add, entry, file});
}

void Transaction::registerFile(const HostTicket &reg)
{
  if(!AddRemoveReaScript)
    return;
  if(reg.file.type != Package::ScriptType || !reg.file.sections)
    return;

  m_regQueue.push(reg);
}

void Transaction::registerQueued()
{
  while(!m_regQueue.empty()) {
    const HostTicket &reg = m_regQueue.front();
    registerScript(reg, m_regQueue.size() == 1);
    m_regQueue.pop();
  }
}

void Transaction::registerScript(const HostTicket &reg, const bool isLastCall)
{
  constexpr std::pair<Source::Section, int> sectionMap[] {
    {Source::MainSection,                0},
    {Source::MIDIEditorSection,          32060},
    {Source::MIDIEventListEditorSection, 32061},
    {Source::MIDIInlineEditorSection,    32062},
    {Source::MediaExplorerSection,       32063},
  };

  const std::string &fullPath = reg.file.path.prependRoot().join();

  std::vector<int> sections;

  for(auto &[flag, id] : sectionMap) {
    if(reg.file.sections & flag)
      sections.push_back(id);
  }

  assert(!sections.empty()); // is a section missing in sectionMap?

  bool enableError = reg.add;
  auto it = sections.begin();

  while(true) {
    const int section = *it++;
    const bool isLastSection = it == sections.end();

    const int id = AddRemoveReaScript(reg.add, section, fullPath.c_str(),
      isLastCall && isLastSection);

    if(!id && enableError) {
      m_receipt.addError({"This script could not be registered in REAPER.",
        reg.file.path.join()});
      enableError = false;
    }

    if(isLastSection)
      break;
  }
}

void Transaction::inhibit(const Remote &remote)
{
  // prevents index post-download callbacks from being called
  const auto &it = m_syncedRemotes.find(remote.name());
  if(it != m_syncedRemotes.end())
    m_syncedRemotes.erase(it);
}

void Transaction::promptObsolete()
{
  if(!g_reapack->config()->install.promptObsolete || m_obsolete.empty())
    return;

  std::vector<Registry::Entry> selected;
  selected.insert(selected.end(), m_obsolete.begin(), m_obsolete.end());
  m_obsolete.clear();

  if(!m_promptObsolete(selected) || selected.empty())
    return;

  if(m_taskQueues.empty())
    m_taskQueues.push(TaskQueue());

  for(const auto &entry : selected)
    m_taskQueues.back().push(std::make_shared<UninstallTask>(entry, this));
}
