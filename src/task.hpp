/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2024  Christian Fillion
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

#ifndef REAPACK_TASK_HPP
#define REAPACK_TASK_HPP

#include "config.hpp"
#include "path.hpp"
#include "registry.hpp"
#include "remote.hpp"

#include <memory>
#include <set>
#include <unordered_set>
#include <vector>

class ArchiveReader;
class Index;
class Source;
class ThreadTask;
class Transaction;
class Version;
struct InstallOpts;

typedef std::shared_ptr<ArchiveReader> ArchiveReaderPtr;
typedef std::shared_ptr<const Index> IndexPtr;

class Task {
public:
  Task(Transaction *parent) : m_tx(parent) {}
  virtual ~Task() = default;

  virtual bool start() { return true; }
  virtual void commit() = 0;
  virtual void rollback() {}

  bool operator<(const Task &o) { return priority() < o.priority(); }

protected:
  virtual int priority() const { return 0; }
  Transaction *tx() const { return m_tx; }

private:
  Transaction *m_tx;
};

class SynchronizeTask : public Task {
public:
  // TODO: remove InstallOpts argument
  SynchronizeTask(const Remote &remote, bool stale, bool fullSync,
    const InstallOpts &, Transaction *);

protected:
  bool start() override;
  void commit() override;

private:
  void synchronize(const Package *);

  Remote m_remote;
  Path m_indexPath;
  InstallOpts m_opts;
  bool m_stale;
  bool m_fullSync;
};

class InstallTask : public Task {
public:
  InstallTask(const Version *ver, int flags, const Registry::Entry &,
    const ArchiveReaderPtr &, Transaction *);

  bool start() override;
  void commit() override;
  void rollback() override;

private:
  void push(ThreadTask *, const TempPath &);

  const Version *m_version;
  int m_flags;
  Registry::Entry m_oldEntry;
  ArchiveReaderPtr m_reader;
  bool m_symlink;

  bool m_fail;
  IndexPtr m_index; // keep in memory
  std::vector<Registry::File> m_oldFiles;
  std::vector<TempPath> m_newFiles;
  std::unordered_set<ThreadTask *> m_waiting;
};

class UninstallTask : public Task {
public:
  UninstallTask(const Registry::Entry &, Transaction *);

protected:
  int priority() const override { return 1; }
  bool start() override;
  void commit() override;

private:
  Registry::Entry m_entry;
  std::vector<Registry::File> m_files;
  std::set<Path> m_removedFiles;
};

class FlagsTask : public Task {
public:
  FlagsTask(const Registry::Entry &, int flags, Transaction *);

protected:
  void commit() override;

private:
  Registry::Entry m_entry;
  int m_flags;
};

class ExportTask : public Task {
public:
  ExportTask(const std::string &path, Transaction *);

protected:
  bool start() override;
  void commit() override;
  void rollback() override;

private:
  TempPath m_path;
};

#endif
