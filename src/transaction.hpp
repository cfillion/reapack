/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2018  Christian Fillion
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

#include "config.hpp"
#include "event.hpp"
#include "registry.hpp"

#include <list>
#include <optional>

class FileDownload;
class Index;
class Package;
class Remote;
class ThreadPool;

class Transaction {
public:
  void synchronize(const RemotePtr &,
    const std::optional<bool> forceAutoInstall = std::nullopt);
  void run(Registry *, ThreadPool *);

  AsyncEvent<void(std::string)> onSetStatusAsync;
  AsyncEvent<void()> onAddStepAsync;
  AsyncEvent<void()> onStepFinishedAsync;
  AsyncEvent<bool(std::vector<Registry::Entry> *)> onPromptObsoleteAsync;

private:
  class Synchronize {
  public:
    Synchronize(const std::shared_ptr<Remote> &, const InstallOpts &, Transaction *);

    bool lockRemote();
    void fetch();
    void process(std::vector<Registry::Entry> &obsolete);

  private:
    std::shared_ptr<const Index> loadIndex();
    void synchronize(const Package *);

    std::weak_ptr<Remote> m_weakRemote;
    InstallOpts m_opts;
    std::shared_ptr<Remote> m_remote;
    std::shared_ptr<FileDownload> m_download;

    Transaction *m_tx;
  };

  bool synchronize();

  std::list<Synchronize> m_synchronize;

  // EventSink *m_notify;
  Registry *m_registry;
  ThreadPool *m_threadPool;
};

#if 1 == 0
#include "receipt.hpp"
#include "registry.hpp"
#include "task.hpp"
#include "thread.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <unordered_set>

class ArchiveReader;
class InstallTask;
class Path;
class Remote;
class SynchronizeTask;
class UninstallTask;

typedef std::shared_ptr<Task> TaskPtr;
typedef std::shared_ptr<Remote> RemotePtr;

struct HostTicket { bool add; Registry::Entry entry; Registry::File file; };

class Transaction {
public:
  typedef std::function<void()> CleanupHandler;
  typedef std::function<bool(std::vector<Registry::Entry> &)> ObsoleteHandler;

  Transaction();

  void setCleanupHandler(const CleanupHandler &cb) { m_cleanupHandler = cb; }
  void setObsoleteHandler(const ObsoleteHandler &cb) { m_promptObsolete = cb; }

  void fetchIndex(const RemotePtr &, bool stale = false);
  void synchronize(const RemotePtr &,
    const std::optional<bool> &forceAutoInstall = std::nullopt);
  void install(const Version *, bool pin = false, const ArchiveReaderPtr & = nullptr);
  void install(const Version *, const Registry::Entry &oldEntry,
    bool pin = false, const ArchiveReaderPtr & = nullptr);
  void setPinned(const Registry::Entry &, bool pinned);
  void uninstall(const RemotePtr &);
  void uninstall(const Registry::Entry &);
  void exportArchive(const std::string &path);
  bool runTasks();

  bool isCancelled() const { return m_isCancelled; }

  Receipt *receipt() { return &m_receipt; }
  Registry *registry() { return &m_registry; }
  ThreadPool *threadPool() { return &m_threadPool; }

  Event<void()> onFinish;

protected:
  friend SynchronizeTask;
  friend InstallTask;
  friend UninstallTask;

  void addObsolete(const Registry::Entry &e) { m_obsolete.insert(e); }
  void registerAll(bool add, const Registry::Entry &);
  void registerFile(const HostTicket &t) { m_regQueue.push(t); }

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

  void registerQueued();
  void registerScript(const HostTicket &, bool isLast);
  void keepAlive(const IndexPtr &i) { m_keepAlive.push_back(i); }
  void promptObsolete();
  void runQueue(TaskQueue &queue);
  bool commitTasks();
  void finish();

  bool m_isCancelled;
  Registry m_registry;
  Receipt m_receipt;

  std::vector<IndexPtr> m_keepAlive;
  std::unordered_set<Registry::Entry> m_obsolete;

  ThreadPool m_threadPool;
  TaskQueue m_nextQueue;
  std::queue<TaskQueue> m_taskQueues;
  std::queue<TaskPtr> m_runningTasks;
  std::queue<HostTicket> m_regQueue;

  CleanupHandler m_cleanupHandler;
  ObsoleteHandler m_promptObsolete;
};
#endif

#endif
