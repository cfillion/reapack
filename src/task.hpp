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

#ifndef REAPACK_TASK_HPP
#define REAPACK_TASK_HPP

#include <boost/signals2.hpp>
#include <vector>

class Download;
class Source;
class Transaction;
class Path;
class Version;

class Task {
public:
  typedef boost::signals2::signal<void ()> Signal;
  typedef Signal::slot_type Callback;

  Task(Transaction *parent);

  void onCommit(const Callback &callback) { m_onCommit.connect(callback); }
  void onFinish(const Callback &callback) { m_onFinish.connect(callback); }

  void install(Version *ver);
  void commit();
  void cancel();

private:
  static int RemoveFile(const std::string &);
  static int RenameFile(const std::string &, const std::string &);

  typedef std::pair<Path, Path> PathPair;

  void finish();
  void rollback();

  void saveSource(Download *, Source *);

  Transaction *m_transaction;
  bool m_isCancelled;
  std::vector<Download *> m_remaining;
  std::vector<PathPair> m_files;

  Signal m_onCommit;
  Signal m_onFinish;
};

#endif
