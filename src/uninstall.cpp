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

#include "task.hpp"

#include "config.hpp"
#include "reapack.hpp"
#include "receipt.hpp"
#include "registry.hpp"
#include "index.hpp"
#include "transaction.hpp"
#include "filesystem.hpp"

using namespace std;

UninstallTask::UninstallTask(const Registry::Entry &re, Transaction *tx)
  : Task(tx), m_entry(move(re))
{
}

bool UninstallTask::start()
{
  tx()->registry()->getFiles(m_entry).swap(m_files);

  // allow conflicting packages to be installed
  tx()->registry()->forget(m_entry);

  return true;
}

void UninstallTask::commit()
{
  for(const auto &file : m_files) {
    if(!FS::exists(file.path) || FS::removeRecursive(file.path))
      tx()->receipt()->addRemoval(file.path);
    else
      tx()->receipt()->addError({FS::lastError(), file.path.join()});

    tx()->registerFile({false, m_entry, file});
  }

  tx()->registry()->forget(m_entry);
}

UninstallRemoteTask::UninstallRemoteTask(const RemotePtr &remote, Transaction *tx)
  : Task(tx), m_remote(remote)
{
  for(const auto &entry : tx->registry()->getEntries(remote->name()))
    m_subTasks.push_back(make_unique<UninstallTask>(entry, tx));
}

bool UninstallRemoteTask::start()
{
  for(const auto &task : m_subTasks)
    task->start();

  return true;
}

void UninstallRemoteTask::commit()
{
  for(const auto &task : m_subTasks)
    task->commit();

  FS::remove(Index::pathFor(m_remote->name()));
  g_reapack->config()->remotes.remove(m_remote);
}
