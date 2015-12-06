#include "transaction.hpp"

#include "errors.hpp"

#include <fstream>

#include <reaper_plugin_functions.h>

using namespace std;

Transaction::Transaction(Registry *reg, const Path &root)
  : m_registry(reg), m_root(root)
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
      addError(dl->contents(), dl->name());
      return;
    }

    const Path path = m_dbPath + ("remote_" + dl->name() + ".xml");
    ofstream file(path.join());

    if(file.bad()) {
      addError(strerror(errno), dl->name());
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
      addError(e.what(), dl->name());
    }
  });

  m_queue.push(dl);

  // execute prepare after the download is deleted, in case finish is called
  // the queue will also not contain the download anymore
  dl->onFinish(bind(&Transaction::prepare, this));
}

void Transaction::prepare()
{
  if(!m_queue.empty())
    return;

  for(Database *db : m_databases) {
    for(Package *pkg : db->packages()) {
      if(m_registry->versionOf(pkg) != pkg->lastVersion()->name())
        m_packages.push_back(pkg);
    }
  }

  if(m_packages.empty()) {
    ShowMessageBox("Nothing to do!", "ReaPack", 0);
    finish();
    return;
  }

  m_onReady();
}

void Transaction::run()
{
  for(Package *pkg : m_packages) {
    try {
      install(pkg);
    }
    catch(const reapack_error &e) {
      addError(e.what(), pkg->name());
    }
  }
}

void Transaction::cancel()
{
  ShowMessageBox("Not Implemented", "Cancel transaction", 0);
}

void Transaction::install(Package *pkg)
{
  const string &url = pkg->lastVersion()->source(0)->url();
  const Path path = m_root + pkg->targetPath();
  const string dbName = pkg->category()->database()->name();

  Download *dl = new Download(dbName + "/" + pkg->name(), url);
  dl->onFinish([=] {
    if(dl->status() != 200) {
      addError(dl->contents(), dl->name());
      return;
    }

    RecursiveCreateDirectory(path.dirname().c_str(), 0);

    ofstream file(path.join());
    if(file.bad()) {
      addError(strerror(errno), dl->name());
      return;
    }

    file << dl->contents();
    file.close();

    m_registry->push(pkg);
  });

  m_queue.push(dl);

  // execute finish after the download is deleted
  // this prevents the download queue from being deleted before the download
  dl->onFinish(bind(&Transaction::finish, this));
}

void Transaction::finish()
{
  if(!m_queue.empty())
    return;

  m_onFinish();
}

void Transaction::addError(const string &message, const string &title)
{
  // ShowMessageBox(message.c_str(), title.c_str(), 0);
}
