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

#ifndef REAPACK_TRANSACTION_HPP
#define REAPACK_TRANSACTION_HPP

#include "download.hpp"
#include "receipt.hpp"
#include "registry.hpp"
#include "task.hpp"

#include <boost/optional.hpp>
#include <boost/signals2.hpp>
#include <functional>
#include <memory>
#include <set>
#include <unordered_set>

class Config;
class Path;
class Remote;
struct InstallOpts;

typedef std::shared_ptr<Task> TaskPtr;

struct HostTicket { bool add; Registry::Entry entry; Registry::File file; };

class Transaction {
public:
  typedef boost::signals2::signal<void ()> VoidSignal;
  typedef std::function<void()> CleanupHandler;

  Transaction(Config *);

  void onFinish(const VoidSignal::slot_type &slot) { m_onFinish.connect(slot); }
  void setCleanupHandler(const CleanupHandler &cb) { m_cleanupHandler = cb; }

  void synchronize(const Remote &,
    boost::optional<bool> forceAutoInstall = boost::none);
  void install(const Version *, bool pin = false);
  void setPinned(const Registry::Entry &, bool pinned);
  void uninstall(const Remote &);
  void uninstall(const Registry::Entry &);
  void registerAll(const Remote &);
  bool runTasks();

  bool isCancelled() const { return m_isCancelled; }

  Receipt *receipt() { return &m_receipt; }
  Registry *registry() { return &m_registry; }
  const Config *config() { return m_config; }
  DownloadQueue *downloadQueue() { return &m_downloadQueue; }

  void registerAll(bool add, const Registry::Entry &);
  void registerFile(const HostTicket &t) { m_regQueue.push(t); }
  bool saveFile(Download *, const Path &);

private:
  class CompareTask {
  public:
    bool operator()(const TaskPtr &l, const TaskPtr &r) const
    {
      return *l < *r;
    }
  };

  typedef std::priority_queue<TaskPtr,
    std::vector<TaskPtr>, CompareTask> TaskQueue;

  void fetchIndex(const Remote &, const std::function<void ()> &);
  void synchronize(const Package *, const InstallOpts &);
  bool allFilesExists(const std::set<Path> &) const;
  void registerQueued();
  void registerScript(const HostTicket &);
  void inhibit(const Remote &);
  bool commitTasks();
  void finish();

  bool m_isCancelled;
  const Config *m_config;
  Registry m_registry;
  Receipt m_receipt;

  std::unordered_set<std::string> m_syncedRemotes;
  std::unordered_set<std::string> m_inhibited;

  DownloadQueue m_downloadQueue;
  TaskQueue m_currentQueue;
  std::queue<TaskQueue> m_taskQueues;
  std::queue<TaskPtr> m_runningTasks;
  std::queue<HostTicket> m_regQueue;

  VoidSignal m_onFinish;
  CleanupHandler m_cleanupHandler;
};

#endif
