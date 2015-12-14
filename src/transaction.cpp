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

  vector<Path> allFiles;

  for(Database *db : m_databases) {
    for(Package *pkg : db->packages()) {
      vector<Path> files = pkg->lastVersion()->files();
      allFiles.insert(allFiles.end(), files.begin(), files.end());

      const auto uniqueIt = unique(allFiles.begin(), allFiles.end());
      if(uniqueIt != allFiles.end()) {
        for(auto it = uniqueIt; it != allFiles.end(); it++) {
          addError("Conflict: This file is created by more than one package",
            it->join());
        }

        finish();
        return;
      }

      bool exists = true;

      for(const Path &path : files) {
        if(!file_exists(prefixPath(path).join().c_str())) {
          exists = false;
          break;
        }
      }

      Registry::QueryResult entry = m_registry->query(pkg);

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
