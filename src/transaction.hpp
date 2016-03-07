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
#include "path.hpp"
#include "receipt.hpp"
#include "registry.hpp"

#include <boost/signals2.hpp>
#include <functional>
#include <set>

class Remote;
class Index;
class Task;

struct InstallTicket {
  enum Type { Install, Upgrade };

  Type type;
  const Version *version;
  const Registry::Entry regEntry;
};

struct HostTicket { bool add; Registry::Entry entry; std::string file; };

class Transaction {
public:
  typedef boost::signals2::signal<void ()> Signal;
  typedef Signal::slot_type Callback;

  Transaction();
  ~Transaction();

  void onFinish(const Callback &callback) { m_onFinish.connect(callback); }
  void onDestroy(const Callback &callback) { m_onDestroy.connect(callback); }

  void synchronize(const Remote &, bool userAction = true);
  void uninstall(const Remote &);
  void registerAll(const Remote &);
  void unregisterAll(const Remote &);
  void runTasks();

  bool isCancelled() const { return m_isCancelled; }
  const Receipt *receipt() const { return &m_receipt; }
  size_t taskCount() const { return m_tasks.size(); }

  DownloadQueue *downloadQueue() { return &m_downloadQueue; }

  bool saveFile(Download *, const Path &);
  void addError(const std::string &msg, const std::string &title);

private:
  typedef std::function<void ()> IndexCallback;

  void fetchIndex(const Remote &, const IndexCallback &cb);
  void saveIndex(Download *, const std::string &remoteName);

  void upgrade(const Package *pkg);
  void installQueued();
  void installTicket(const InstallTicket &);
  void addTask(Task *);

  bool allFilesExists(const std::set<Path> &) const;

  void registerInHost(bool add, const Registry::Entry &);
  void registerQueued();
  void registerScript(const HostTicket &);

  void finish();
  void inhibit(const Remote &);

  bool m_isCancelled;
  Registry *m_registry;
  Receipt m_receipt;

  std::multimap<std::string, IndexCallback> m_remotes;
  std::vector<const Index *> m_indexes;
  std::vector<Task *> m_tasks;

  DownloadQueue m_downloadQueue;
  std::queue<Task *> m_taskQueue;
  std::queue<InstallTicket> m_installQueue;
  std::queue<HostTicket> m_regQueue;

  Signal m_onFinish;
  Signal m_onDestroy;
};

#endif
