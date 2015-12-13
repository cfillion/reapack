#include "transaction.hpp"

#include "errors.hpp"

#include <cstdio>
#include <fstream>

#include <reaper_plugin_functions.h>

using namespace std;

Transaction::Transaction(Registry *reg, const Path &root)
  : m_registry(reg), m_root(root), m_isCancelled(false)
{
  m_dbPath = m_root + "ReaPack";

  RecursiveCreateDirectory(m_dbPath.join().c_str(), 0);
}

Transaction::~Transaction()
{
  for(PackageTransaction *tr : m_transactions)
    delete tr;

  for(Database *db : m_databases)
    delete db;
}

void Transaction::fetch(const RemoteMap &remotes)
{
  for(const Remote &remote : remotes)
    fetch(remote);
}

void Transaction::fetch(const Remote &remote)
{
  Download *dl = new Download(remote.first, remote.second);
  dl->onFinish(bind(&Transaction::saveDatabase, this, dl));

  m_queue.push(dl);

  // execute prepare after the download is deleted, in case finish is called
  // the queue will also not contain the download anymore
  dl->onFinish(bind(&Transaction::prepare, this));
}

void Transaction::saveDatabase(Download *dl)
{
  const Path path = m_dbPath + ("remote_" + dl->name() + ".xml");

  if(!saveFile(dl, path))
    return;

  try {
    Database *db = Database::load(path.join().c_str());
    db->setName(dl->name());

    m_databases.push_back(db);
  }
  catch(const reapack_error &e) {
    addError(e.what(), dl->url());
  }
}

void Transaction::prepare()
{
  if(!m_queue.idle())
    return;

  for(Database *db : m_databases) {
    for(Package *pkg : db->packages()) {
      Registry::QueryResult entry = m_registry->query(pkg);
      bool exists = PackageTransaction::isInstalled(pkg->lastVersion(), m_root);

      if(entry.status == Registry::UpToDate && exists)
        continue;

      m_packages.push_back({pkg, entry});
    }
  }

  if(m_packages.empty())
    finish();
  else
    m_onReady();
}

void Transaction::run()
{
  for(const PackageEntry &entry : m_packages) {
    Package *pkg = entry.first;
    Registry::QueryResult regEntry = entry.second;

    PackageTransaction *tr = new PackageTransaction(this);

    try {
      tr->install(entry.first->lastVersion());
      tr->onFinish([=] {
        if(regEntry.status == Registry::UpdateAvailable)
          m_updates.push_back(entry);
        else
          m_new.push_back(entry);

        m_registry->push(pkg);

        finish();
      });

      m_transactions.push_back(tr);
    }
    catch(const reapack_error &e) {
      addError(e.what(), pkg->fullName());
      delete tr;
    }
  }
}

void Transaction::cancel()
{
  m_isCancelled = true;

  for(PackageTransaction *tr : m_transactions)
    tr->cancel();

  m_queue.abort();
}

bool Transaction::saveFile(Download *dl, const Path &path)
{
  if(dl->status() != 200) {
    addError(dl->contents(), dl->url());
    return false;
  }

  RecursiveCreateDirectory(path.dirname().c_str(), 0);

  const string strPath = path.join();
  ofstream file(strPath);

  if(file.bad()) {
    addError(strerror(errno), strPath);
    return false;
  }

  file << dl->contents();
  file.close();

  return true;
}

void Transaction::finish()
{
  if(!m_queue.idle())
    return;

  if(!m_isCancelled) {
    for(PackageTransaction *tr : m_transactions)
      tr->apply();
  }

  m_onFinish();
}

void Transaction::addError(const string &message, const string &title)
{
  m_errors.push_back({message, title});
}

Path Transaction::prefixPath(const Path &input) const
{
  return m_root + input;
}

bool PackageTransaction::isInstalled(Version *ver, const Path &root)
{
  // TODO
  // file_exists(installPath(pkg).join().c_str());
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

  m_files.push({tmpPath, targetPath});

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

void PackageTransaction::apply()
{
  while(!m_files.empty()) {
    const PathPair paths = m_files.front();
    m_files.pop();

    const string tempPath = m_transaction->prefixPath(paths.first).join();
    const string targetPath = m_transaction->prefixPath(paths.second).join();

    if(rename(tempPath.c_str(), targetPath.c_str()))
      m_transaction->addError(strerror(errno), targetPath);
  }
}

void PackageTransaction::rollback()
{
  while(!m_files.empty()) {
    const PathPair paths = m_files.front();
    m_files.pop();

    const string tempPath = m_transaction->prefixPath(paths.first).join();

    if(remove(tempPath.c_str()))
      m_transaction->addError(strerror(errno), tempPath);
  }
}
