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

#include "config.hpp"
#include "download.hpp"
#include "receipt.hpp"
#include "registry.hpp"

#include <boost/signals2.hpp>
#include <functional>
#include <memory>
#include <set>
#include <unordered_set>

class Index;
class Path;
class Remote;
class Task;

typedef std::shared_ptr<const Index> IndexPtr;

struct HostTicket { bool add; Registry::Entry entry; std::string file; };

class Transaction {
public:
  typedef boost::signals2::signal<void ()> VoidSignal;
  typedef std::function<void()> CleanupHandler;

  Transaction(const InstallOpts &);
  ~Transaction();

  void onFinish(const VoidSignal::slot_type &slot) { m_onFinish.connect(slot); }
  void setCleanupHandler(const CleanupHandler &cb) { m_cleanupHandler = cb; }

  void synchronize(const Remote &);
  void install(const Version *);
  void setPinned(const Package *, bool pinned);
  void setPinned(const Registry::Entry &, bool pinned);
  void uninstall(const Remote &);
  void uninstall(const Registry::Entry &);
  void registerAll(const Remote &);
  bool runTasks();

  bool isCancelled() const { return m_isCancelled; }
  const Receipt &receipt() const { return m_receipt; }
  size_t taskCount() const { return m_tasks.size(); }

  DownloadQueue *downloadQueue() { return &m_downloadQueue; }
  InstallOpts *options() { return &m_opts; }

  bool saveFile(Download *, const Path &);
  void addError(const std::string &msg, const std::string &title);

private:
  typedef std::function<void ()> IndexCallback;

  void fetchIndex(const Remote &, const IndexCallback &cb);
  void saveIndex(Download *, const std::string &remoteName);

  void synchronize(const Package *);
  bool install(const Version *, const Registry::Entry &);
  bool installDependencies(const Version *);
  void addTask(Task *);

  bool allFilesExists(const std::set<Path> &) const;

  void registerInHost(bool add, const Registry::Entry &);
  void registerQueued();
  void registerScript(const HostTicket &);

  void finish();
  void inhibit(const Remote &);

  bool m_isCancelled;
  InstallOpts m_opts;
  Registry *m_registry;
  Receipt m_receipt;

  std::multimap<std::string, IndexCallback> m_remotes;
  std::unordered_set<std::string> m_inhibited;
  std::unordered_set<IndexPtr> m_indexes;
  std::unordered_set<const Package *> m_deps;
  std::vector<Task *> m_tasks;

  DownloadQueue m_downloadQueue;
  std::queue<Task *> m_taskQueue;
  std::queue<HostTicket> m_regQueue;

  VoidSignal m_onFinish;
  CleanupHandler m_cleanupHandler;
};

#endif
