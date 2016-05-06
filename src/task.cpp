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

#include "filesystem.hpp"
#include "source.hpp"
#include "transaction.hpp"
#include "version.hpp"

using namespace std;

Task::Task(Transaction *transaction)
  : m_transaction(transaction), m_isCancelled(false)
{
}

void Task::start()
{
  doStart();
}

void Task::commit()
{
  if(m_isCancelled)
    return;

  if(doCommit())
    m_onCommit();
}

void Task::rollback()
{
  m_isCancelled = true;

  // it's the transaction queue's job to abort the running downloads, not ours

  doRollback();
}

InstallTask::InstallTask(const Version *ver,
    const set<Path> &oldFiles, Transaction *t)
  : Task(t), m_version(ver), m_oldFiles(move(oldFiles))
{
}

void InstallTask::doStart()
{
  const auto &sources = m_version->sources();

  for(auto it = sources.begin(); it != sources.end();) {
    const Path &path = it->first;
    const Source *src = it->second;

    Download *dl = new Download(src->fullName(), src->url());
    dl->onFinish(bind(&InstallTask::saveSource, this, dl, src));

    transaction()->downloadQueue()->push(dl);

    // skip duplicate files
    do { it++; } while(it != sources.end() && path == it->first);
  }
}

void InstallTask::saveSource(Download *dl, const Source *src)
{
  if(isCancelled())
    return;

  const Path &targetPath = src->targetPath();

  Path tmpPath(targetPath);
  tmpPath[tmpPath.size() - 1] += ".new";

  m_newFiles.push_back({targetPath, tmpPath});

  const auto old = m_oldFiles.find(targetPath);

  if(old != m_oldFiles.end())
    m_oldFiles.erase(old);

  if(!transaction()->saveFile(dl, tmpPath)) {
    rollback();
    return;
  }
}

bool InstallTask::doCommit()
{
  for(const Path &path : m_oldFiles)
    FS::remove(path);

  for(const PathGroup &paths : m_newFiles) {
#ifdef _WIN32
    FS::remove(paths.target);
#endif

    if(!FS::rename(paths.temp, paths.target)) {
      transaction()->addError("Cannot rename to target: " + FS::lastError(),
        paths.target.join());

      // it's a bit late to rollback here as some files might already have been
      // overwritten. at least we can delete the temporary files
      rollback();
      return false;
    }
  }

  return true;
}

void InstallTask::doRollback()
{
  for(const PathGroup &paths : m_newFiles)
    FS::removeRecursive(paths.temp);

  m_newFiles.clear();
}

RemoveTask::RemoveTask(const vector<Path> &files, Transaction *t)
  : Task(t), m_files(move(files))
{
}

bool RemoveTask::doCommit()
{
  for(const Path &path : m_files) {
    if(FS::removeRecursive(path))
      m_removedFiles.insert(path);
    else
      transaction()->addError(FS::lastError(), path.join());
  }

  return true;
}
