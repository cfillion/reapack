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

using namespace std;

Task::Task(Transaction *transaction)
  : m_transaction(transaction), m_isCancelled(false)
{
}

void Task::install(Version *ver, const set<Path> &oldFiles)
{
  const auto &sources = ver->sources();

  for(auto it = sources.begin(); it != sources.end();) {
    const Path &path = it->first;
    Source *src = it->second;

    Download *dl = new Download(src->fullName(), src->url());
    dl->onFinish(bind(&Task::saveSource, this, dl, src));

    m_transaction->downloadQueue()->push(dl);

    // skip duplicate files
    do { it++; } while(it != sources.end() && path == it->first);
  }

  m_oldFiles = move(oldFiles);
}

void Task::saveSource(Download *dl, Source *src)
{
  if(m_isCancelled)
    return;

  const Path targetPath = src->targetPath();
  Path tmpPath = targetPath;
  tmpPath[tmpPath.size() - 1] += ".new";

  m_files.push_back({tmpPath, targetPath});

  const auto old = m_oldFiles.find(targetPath);
  if(old != m_oldFiles.end())
    m_oldFiles.erase(old);

  const Path path = m_transaction->prefixPath(tmpPath);

  if(!m_transaction->saveFile(dl, path)) {
    cancel();
    return;
  }
}

void Task::cancel()
{
  m_isCancelled = true;

  // it's the transaction queue's job to abort the running downloads, not ours

  rollback();
}

void Task::commit()
{
  if(m_isCancelled)
    return;

  for(const Path &path : m_oldFiles)
    removeFile(path);

  for(const PathPair &paths : m_files) {
    removeFile(paths.second);

    if(renameFile(paths.first, paths.second)) {
      m_transaction->addError(strerror(errno), paths.first.join());

      // it's a bit late to rollback here as some files might already have been
      // overwritten. at least we can delete the temporary files
      rollback();
      return;
    }
  }

  m_onCommit();
}

void Task::rollback()
{
  for(const PathPair &paths : m_files)
    removeFile(paths.first);

  m_files.clear();
}

int Task::removeFile(const Path &path) const
{
  const string &fullPath = m_transaction->prefixPath(path).join();

#ifdef _WIN32
  return _wremove(make_autostring(fullPath).c_str());
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
