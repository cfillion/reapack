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

#include "transaction.hpp"

#include "download.hpp"
#include "errors.hpp"
#include "filesystem.hpp"
#include "index.hpp"
#include "reapack.hpp"
#include "registry.hpp"
#include "thread.hpp"

bool Transaction::synchronize()
{
  printf("before: %zu\n", m_synchronize.size());

  // remove remotes that were removed since the task was queued
  m_synchronize.remove_if([](Synchronize &sync) { return !sync.lockRemote(); });

  printf("after: %zu\n", m_synchronize.size());

  for(Synchronize &sync : m_synchronize)
    sync.fetch();

  printf("waiting\n");
  if(!m_threadPool->wait())
    return false;

  onSetStatusAsync("Processing the package list");

  std::vector<Registry::Entry> obsolete;

  for(Synchronize &sync : m_synchronize)
    sync.process(obsolete);

  if(g_reapack->config()->install.promptObsolete && !obsolete.empty()) {
    if(*onPromptObsoleteAsync(&obsolete).get()) {
      for(const auto &entry : obsolete)
        (void)entry;//uninstall(entry);
    }

    printf("after prompt\n");
  }

  return true;
}

Transaction::Synchronize::Synchronize(const RemotePtr &remote,
    const InstallOpts &opts, Transaction *tx)
  : m_weakRemote(remote), m_opts(opts), m_tx(tx)
{
}

bool Transaction::Synchronize::lockRemote()
{
  m_remote = m_weakRemote.lock();
  return m_remote != nullptr;
}

void Transaction::Synchronize::fetch()
{
  // tx->log(String::format("synchronize: fetching index for %s"));

  // TODO: move to Remote
  const Path &indexPath = Index::pathFor(m_remote->name());
  const auto &netConfig = g_reapack->config()->network;

  // time_t mtime = 0, now = time(nullptr);
  // FS::mtime(indexPath, &mtime);

  // bool m_stale = true;
  // const time_t threshold = netConfig.staleThreshold;
  // if(!m_stale && mtime && (!threshold || mtime > now - threshold))
  //   return;

  m_download = std::make_shared<FileDownload>(indexPath, m_remote->url(),
      netConfig, Download::NoCacheFlag);

  m_download->onStart >> std::bind(std::ref(m_tx->onSetStatusAsync),
    String::format("Fetching %s", m_remote->name().c_str()));

  m_download->onFinish >> std::ref(m_tx->onStepFinishedAsync);

  m_tx->onAddStepAsync();
  m_tx->m_threadPool->push(m_download);
}

void Transaction::Synchronize::process(std::vector<Registry::Entry> &obsolete)
{
  printf("dl = %p\n", m_download.get());
  if(m_download->failed()) {
    printf("download failed: %s\n", m_download->errorMessage().c_str());
    return;
  }

  printf("saving download\n");
  m_download->save();

  const IndexPtr &index = loadIndex();

  if(!index)
    return;

  for(const Package *pkg : index->packages())
    synchronize(pkg);

  if(m_opts.promptObsolete && !m_remote->test(Remote::ProtectedFlag)) {
    for(const auto &entry : m_tx->m_registry->getEntries(m_remote->name())) {
      if(!entry.pinned && !index->find(entry.category, entry.package))
        obsolete.push_back(entry);
    }
  }
}

IndexPtr Transaction::Synchronize::loadIndex()
{
  try {
    return m_remote->loadIndex();
  }
  catch(const reapack_error &) {
    printf("could not load repository\n");
    // tx()->receipt()->addError({
    //   String::format("Could not load repository: %s", e.what()), m_remote->name()});
    return nullptr;
  }
}

void Transaction::Synchronize::synchronize(const Package *pkg)
{
  const auto &entry = m_tx->m_registry->getEntry(pkg);

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

  printf("installing %s\n", latest->name().toString().c_str());
  // m_tx->install(latest, entry);
}
