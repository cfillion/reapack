#include "transaction.hpp"

#include "errors.hpp"

#include <fstream>

#include <reaper_plugin_functions.h>

using namespace std;

Transaction::Transaction(Registry *reg, const Path &root)
  : m_registry(reg), m_root(root), m_new(0), m_updates(0)
{
  m_dbPath = m_root + "ReaPack";
  RecursiveCreateDirectory(m_dbPath.join().c_str(), 0);
}

Transaction::~Transaction()
{
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
  dl->onFinish([=]() {
    if(dl->status() != 200) {
      addError(dl->contents(), dl->url());
      return;
    }

    const Path path = m_dbPath + ("remote_" + dl->name() + ".xml");
    ofstream file(path.join());

    if(file.bad()) {
      addError(strerror(errno), path.join());
      return;
    }

    file << dl->contents();
    file.close();

    try {
      Database *db = Database::load(path.join().c_str());
      db->setName(dl->name());

      m_databases.push_back(db);
    }
    catch(const reapack_error &e) {
      addError(e.what(), dl->url());
    }
  });

  m_queue.push(dl);

  // execute prepare after the download is deleted, in case finish is called
  // the queue will also not contain the download anymore
  dl->onFinish(bind(&Transaction::prepare, this));
}

void Transaction::prepare()
{
  if(!m_queue.idle())
    return;

  for(Database *db : m_databases) {
    for(Package *pkg : db->packages()) {
      Registry::QueryResult entry = m_registry->query(pkg);
      bool exists = file_exists(installPath(pkg).join().c_str());

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
  for(const PackageEntry &pkg : m_packages) {
    try {
      install(pkg);
    }
    catch(const reapack_error &e) {
      addError(e.what(), pkg.first->fullName());
    }
  }
}

void Transaction::cancel()
{
  m_queue.abort();
}

void Transaction::install(const PackageEntry &pkgEntry)
{
  Package *pkg = pkgEntry.first;
  Version *ver = pkg->lastVersion();

  const Path path = installPath(pkg);
  const Registry::QueryResult regEntry = pkgEntry.second;

  Download *dl = new Download(ver->fullName(), ver->source(0)->url());
  dl->onFinish([=] {
    if(dl->status() != 200) {
      addError(dl->contents(), dl->url());
      return;
    }

    RecursiveCreateDirectory(path.dirname().c_str(), 0);

    ofstream file(path.join());
    if(file.bad()) {
      addError(strerror(errno), path.join());
      return;
    }

    file << dl->contents();
    file.close();

    if(regEntry.status == Registry::UpdateAvailable)
      m_updates.push_back(pkgEntry);
    else
      m_new.push_back(pkgEntry);

    m_registry->push(pkg);
  });

  m_queue.push(dl);

  // execute finish after the download is deleted
  // this prevents the download queue from being deleted before the download is
  dl->onFinish(bind(&Transaction::finish, this));
}

Path Transaction::installPath(Package *pkg) const
{
  return m_root + pkg->targetPath();
}

void Transaction::finish()
{
  if(!m_queue.idle())
    return;

  m_onFinish();
}

void Transaction::addError(const string &message, const string &title)
{
  m_errors.push_back({message, title});
}
