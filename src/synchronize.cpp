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
#include "download.hpp"
#include "filesystem.hpp"
#include "index.hpp"
#include "reapack.hpp"
#include "transaction.hpp"

SynchronizeTask::SynchronizeTask(const RemotePtr &remote, const bool stale,
    const bool fullSync, const InstallOpts &opts, Transaction *tx)
  : Task(tx), m_remote(remote),
    m_opts(opts), m_stale(stale), m_fullSync(fullSync)
{
}

bool SynchronizeTask::start()
{
  const Path &indexPath = Index::pathFor(m_remote->name());
  const auto &netConfig = g_reapack->config()->network;

  time_t mtime = 0, now = time(nullptr);
  FS::mtime(indexPath, &mtime);

  const time_t threshold = netConfig.staleThreshold;
  if(!m_stale && mtime && (!threshold || mtime > now - threshold))
    return true;

  auto dl = new FileDownload(indexPath, m_remote->url(),
    netConfig, Download::NoCacheFlag);
  dl->setName(m_remote->name());

  dl->onFinishAsync >> [=] {
    if(dl->save())
      tx()->receipt()->setIndexChanged();
  };

  tx()->threadPool()->push(dl);

  return true;
}

void SynchronizeTask::commit()
{
  const IndexPtr &index = loadIndex();

  if(!index || !m_fullSync)
    return;

  for(const Package *pkg : index->packages())
    synchronize(pkg);

  if(m_opts.promptObsolete && !m_remote->test(Remote::ProtectedFlag)) {
    for(const auto &entry : tx()->registry()->getEntries(m_remote->name())) {
      if(!entry.pinned && !index->find(entry.category, entry.package))
        tx()->addObsolete(entry);
    }
  }
}

IndexPtr SynchronizeTask::loadIndex()
{
  try {
    IndexPtr index;

    if(!m_stale)
      index = m_remote->index();

    if(!index)
      index = m_remote->loadIndex();

    tx()->keepAlive(index);

    return index;
  }
  catch(const reapack_error &e) {
    tx()->receipt()->addError({
      String::format("Could not load repository: %s", e.what()), m_remote->name()});
    return nullptr;
  }
}

void SynchronizeTask::synchronize(const Package *pkg)
{
  const auto &entry = tx()->registry()->getEntry(pkg);

  if(!entry && !m_opts.autoInstall)
    return;

  const Version *latest = pkg->lastVersion(m_opts.bleedingEdge, entry.version);

  if(!latest)
    return;

  if(entry.version == latest->name()) {
    if(FS::allExists(latest->files()))
      return; // latest version is really installed, nothing to do here!
  }
  else if(entry.pinned || latest->name() < entry.version)
    return;

  tx()->install(latest, entry);
}
