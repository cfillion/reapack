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

#include "task.hpp"

#include "encoding.hpp"
#include "path.hpp"
#include "transaction.hpp"
#include "version.hpp"

#include <cerrno>
#include <cstdio>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

Task::Task(Transaction *transaction)
  : m_transaction(transaction), m_isCancelled(false)
{
}

void Task::rollback()
{
  m_isCancelled = true;

  // it's the transaction queue's job to abort the running downloads, not ours

  doRollback();
}

void Task::commit()
{
  if(m_isCancelled)
    return;

  doCommit();

  m_onCommit();
}

int Task::removeFile(const Path &path) const
{
  const auto_string &fullPath =
    make_autostring(m_transaction->prefixPath(path).join());

#ifdef _WIN32
  if(GetFileAttributes(fullPath.c_str()) & FILE_ATTRIBUTE_DIRECTORY)
    return !RemoveDirectory(fullPath.c_str());
  else
    return _wremove(fullPath.c_str());
#else
  return remove(fullPath.c_str());
#endif
}

int Task::renameFile(const Path &from, const Path &to) const
{
  const string &fullFrom = m_transaction->prefixPath(from).join();
  const string &fullTo = m_transaction->prefixPath(to).join();

#ifdef _WIN32
  return _wrename(make_autostring(fullFrom).c_str(),
    make_autostring(fullTo).c_str());
#else
  return rename(fullFrom.c_str(), fullTo.c_str());
#endif
}

InstallTask::InstallTask(Version *ver, const set<Path> &oldFiles, Transaction *t)
  : Task(t), m_oldFiles(move(oldFiles))
{
  const auto &sources = ver->sources();

  for(auto it = sources.begin(); it != sources.end();) {
    const Path &path = it->first;
    Source *src = it->second;

    Download *dl = new Download(src->fullName(), src->url());
    dl->onFinish(bind(&InstallTask::saveSource, this, dl, src));

    transaction()->downloadQueue()->push(dl);

    // skip duplicate files
    do { it++; } while(it != sources.end() && path == it->first);
  }
}

void InstallTask::saveSource(Download *dl, Source *src)
{
  if(isCancelled())
    return;

  const Path &targetPath = src->targetPath();
  Path tmpPath(targetPath);
  tmpPath[tmpPath.size() - 1] += ".new";

  m_newFiles.push_back({tmpPath, targetPath});

  const auto old = m_oldFiles.find(targetPath);
  if(old != m_oldFiles.end())
    m_oldFiles.erase(old);

  const Path &path = transaction()->prefixPath(tmpPath);

  if(!transaction()->saveFile(dl, path)) {
    rollback();
    return;
  }
}

void InstallTask::doCommit()
{
  for(const Path &path : m_oldFiles)
    removeFile(path);

  for(const PathPair &paths : m_newFiles) {
    removeFile(paths.second);

    if(renameFile(paths.first, paths.second)) {
      transaction()->addError(strerror(errno), paths.first.join());

      // it's a bit late to rollback here as some files might already have been
      // overwritten. at least we can delete the temporary files
      rollback();
      return;
    }
  }
}

void InstallTask::doRollback()
{
  for(const PathPair &paths : m_newFiles)
    removeFile(paths.first);

  m_newFiles.clear();
}

RemoveTask::RemoveTask(const vector<Path> &files, Transaction *t)
  : Task(t), m_files(move(files))
{
}

void RemoveTask::doCommit()
{
  for(const Path &path : m_files)
    remove(path);
}

void RemoveTask::remove(const Path &file)
{
  if(removeFile(file)) {
    transaction()->addError(strerror(errno), file.join());
    return;
  }
  else
    m_removedFiles.push_back(file);

  Path dir = file;

  // remove empty directories, but not top-level ones that were created by REAPER
  while(dir.size() > 2) {
    dir.removeLast();

    if(removeFile(dir))
      break;
  }
}
