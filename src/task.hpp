/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2017  Christian Fillion
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

#include "path.hpp"
#include "registry.hpp"

#include <set>
#include <unordered_set>
#include <vector>

class Download;
class Index;
class Source;
class Transaction;
class Version;

typedef std::shared_ptr<const Index> IndexPtr;

class Task {
public:
  Task(Transaction *parent);
  virtual ~Task() {}

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

class InstallTask : public Task {
public:
  InstallTask(const Version *ver, bool pin, const Registry::Entry &, Transaction *);

  bool start() override;
  void commit() override;
  void rollback() override;

private:
  struct PathGroup { Path target; Path temp; };

  void saveSource(Download *, const Source *);

  const Version *m_version;
  bool m_pin;
  Registry::Entry m_oldEntry;
  bool m_fail;
  IndexPtr m_index; // keep in memory
  std::unordered_set<Download *> m_waiting;
  std::vector<Registry::File> m_oldFiles;
  std::vector<PathGroup> m_newFiles;
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

class PinTask : public Task {
public:
  PinTask(const Registry::Entry &, bool pin, Transaction *);

protected:
  void commit() override;

private:
  Registry::Entry m_entry;
  bool m_pin;
};

#endif
