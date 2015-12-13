#include "pkgtransaction.hpp"

#include "path.hpp"
#include "transaction.hpp"

#include <cerrno>
#include <cstdio>

#include <reaper_plugin_functions.h>

using namespace std;

bool PackageTransaction::isInstalled(Version *ver, Transaction *tr)
{
  for(Source *src : ver->sources()) {
    if(!file_exists(tr->prefixPath(src->targetPath()).join().c_str()))
      return false;
  }

  return true;
}

PackageTransaction::PackageTransaction(Transaction *transaction)
  : m_transaction(transaction), m_isCancelled(false)
{
}

void PackageTransaction::install(Version *ver)
{
  const size_t sourcesSize = ver->sources().size();

  for(size_t i = 0; i < sourcesSize; i++) {
    Source *src = ver->source(i);

    Download *dl = new Download(src->fullName(), src->url());
    dl->onFinish(bind(&PackageTransaction::saveSource, this, dl, src));

    m_remaining.push_back(dl);
    m_transaction->downloadQueue()->push(dl);

    // executing finish after the download is deleted
    // prevents the download queue from being deleted before the download is
    dl->onFinish(bind(&PackageTransaction::finish, this));
  }
}

void PackageTransaction::saveSource(Download *dl, Source *src)
{
  m_remaining.erase(remove(m_remaining.begin(), m_remaining.end(), dl));

  if(m_isCancelled)
    return;

  const Path targetPath = src->targetPath();
  Path tmpPath = targetPath;
  tmpPath[tmpPath.size() - 1] += ".new";

  m_files.push_back({tmpPath, targetPath});

  const Path path = m_transaction->prefixPath(tmpPath);

  if(!m_transaction->saveFile(dl, path)) {
    cancel();
    return;
  }
}

void PackageTransaction::finish()
{
  if(!m_remaining.empty())
    return;

  m_onFinish();
}

void PackageTransaction::cancel()
{
  m_isCancelled = true;

  for(Download *dl : m_remaining)
    dl->abort();

  rollback();
}

void PackageTransaction::commit()
{
  for(const PathPair &paths : m_files) {
    const string tempPath = m_transaction->prefixPath(paths.first).join();
    const string targetPath = m_transaction->prefixPath(paths.second).join();

    if(rename(tempPath.c_str(), targetPath.c_str())) {
      m_transaction->addError(strerror(errno), targetPath);
      rollback();
      return;
    }
  }

  m_files.clear();
  m_onCommit();
}

void PackageTransaction::rollback()
{
  for(const PathPair &paths : m_files) {
    const string tempPath = m_transaction->prefixPath(paths.first).join();

    if(remove(tempPath.c_str()))
      m_transaction->addError(strerror(errno), tempPath);
  }

  m_files.clear();
}
