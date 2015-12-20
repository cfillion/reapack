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

#ifndef REAPACK_TRANSACTION_HPP
#define REAPACK_TRANSACTION_HPP

#include "database.hpp"
#include "download.hpp"
#include "path.hpp"
#include "registry.hpp"
#include "remote.hpp"

#include <boost/signals2.hpp>

class Task;

class Transaction {
public:
  typedef boost::signals2::signal<void ()> Signal;
  typedef Signal::slot_type Callback;

  typedef std::pair<Package *, const Registry::QueryResult> PackageEntry;
  typedef std::vector<PackageEntry> PackageEntryList;

  struct Error {
    std::string message;
    std::string title;
  };

  typedef std::vector<const Error> ErrorList;

  Transaction(Registry *reg, const Path &root);
  ~Transaction();

  void onReady(const Callback &callback) { m_onReady.connect(callback); }
  void onFinish(const Callback &callback) { m_onFinish.connect(callback); }
  void onDestroy(const Callback &callback) { m_onDestroy.connect(callback); }

  void fetch(const RemoteMap &);
  void fetch(const Remote &);
  void run();
  void cancel();

  bool isCancelled() const { return m_isCancelled; }
  DownloadQueue *downloadQueue() { return &m_queue; }
  const PackageEntryList &packages() const { return m_packages; }
  const PackageEntryList &newPackages() const { return m_new; }
  const PackageEntryList &updates() const { return m_updates; }
  const ErrorList &errors() const { return m_errors; }

private:
  friend Task;

  void prepare();
  void finish();

  void saveDatabase(Download *);
  bool saveFile(Download *, const Path &);
  void addError(const std::string &msg, const std::string &title);
  Path prefixPath(const Path &) const;
  bool allFilesExists(const std::set<Path> &) const;
  void registerFiles(const std::set<Path> &);

  Registry *m_registry;

  Path m_root;
  Path m_dbPath;
  bool m_isCancelled;

  DatabaseList m_databases;
  DownloadQueue m_queue;
  PackageEntryList m_packages;
  PackageEntryList m_new;
  PackageEntryList m_updates;
  ErrorList m_errors;

  std::vector<Task *> m_tasks;
  std::set<Path> m_files;
  bool m_hasConflicts;

  Signal m_onReady;
  Signal m_onFinish;
  Signal m_onDestroy;
};

#endif
