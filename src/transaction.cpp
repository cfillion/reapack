#include "transaction.hpp"

#include "errors.hpp"
#include "pkgtransaction.hpp"

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

      vector<Path> files = pkg->lastVersion()->files();
      registerFiles(files);

      if(entry.status == Registry::UpToDate && allFilesExists(files))
        continue;

      m_packages.push_back({pkg, entry});
    }
  }

  if(m_packages.empty() || m_errors.size())
    finish();
  else
    m_onReady();
}

void Transaction::run()
{
  for(const PackageEntry &entry : m_packages) {
    Package *pkg = entry.first;
    const Registry::QueryResult regEntry = entry.second;

    PackageTransaction *tr = new PackageTransaction(this);

    try {
      tr->install(entry.first->lastVersion());
      tr->onCommit([=] {
        if(regEntry.status == Registry::UpdateAvailable)
          m_updates.push_back(entry);
        else
          m_new.push_back(entry);

        m_registry->push(pkg);
      });
      tr->onFinish(bind(&Transaction::finish, this));

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
      tr->commit();
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

bool Transaction::allFilesExists(const vector<Path> &list) const
{
  for(const Path &path : list) {
    if(!file_exists(prefixPath(path).join().c_str()))
      return false;
  }

  return true;
}

void Transaction::registerFiles(const vector<Path> &list)
{
  m_files.insert(m_files.end(), list.begin(), list.end());

  const auto uniqueIt = unique(m_files.begin(), m_files.end());

  for(auto it = uniqueIt; it != m_files.end(); it++) {
    addError("Conflict: This file is part of more than one package",
      it->join());
  }

  m_files.erase(uniqueIt, m_files.end());
}
