/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2022  Christian Fillion
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

SynchronizeTask::SynchronizeTask(const Remote &remote, const bool stale,
    const bool fullSync, const InstallOpts &opts, Transaction *tx)
  : Task(tx), m_remote(remote), m_indexPath(Index::pathFor(m_remote.name())),
    m_opts(opts), m_stale(stale), m_fullSync(fullSync)
{
}

bool SynchronizeTask::start()
{
  const auto &netConfig = g_reapack->config()->network;

  time_t mtime = 0, now = time(nullptr);
  FS::mtime(m_indexPath, &mtime);

  const time_t threshold = netConfig.staleThreshold;
  if(!m_stale && mtime && (!threshold || mtime > now - threshold))
    return true;

  auto dl = new CopyFileDownload(m_indexPath, m_remote.url(),
    netConfig, Download::NoCacheFlag);
  dl->setName(m_remote.name());

  dl->onFinishAsync >> [=] {
    if(dl->save())
      tx()->receipt()->setIndexChanged();
  };

  tx()->threadPool()->push(dl);
  return true;
}

void SynchronizeTask::commit()
{
  if(!FS::exists(m_indexPath))
    return;

  const IndexPtr &index = tx()->loadIndex(m_remote); // TODO: reuse m_indexPath
  if(!index || !m_fullSync)
    return;

  for(const Package *pkg : index->packages())
    synchronize(pkg);

  if(m_opts.promptObsolete && !m_remote.isProtected()) {
    for(const auto &entry : tx()->registry()->getEntries(m_remote.name())) {
      if(!entry.test(Registry::Entry::PinnedFlag) &&
          !index->find(entry.category, entry.package))
        tx()->addObsolete(entry);
    }
  }
}

void SynchronizeTask::synchronize(const Package *pkg)
{
  const auto &entry = tx()->registry()->getEntry(pkg);

  if(!entry && !m_opts.autoInstall)
    return;

  const bool pres = m_opts.bleedingEdge || entry.test(Registry::Entry::BleedingEdgeFlag);
  const Version *latest = pkg->lastVersion(pres, entry.version);

  if(!latest)
    return;

  if(entry.version == latest->name()) {
    if(FS::allExists(latest->files()))
      return; // latest version is really installed, nothing to do here!
  }
  else if(entry.test(Registry::Entry::PinnedFlag) || latest->name() < entry.version)
    return;

  tx()->install(latest, entry);
}
