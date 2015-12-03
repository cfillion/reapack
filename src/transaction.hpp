#ifndef REAPACK_TRANSACTION_HPP
#define REAPACK_TRANSACTION_HPP

#include "database.hpp"
#include "download.hpp"
#include "path.hpp"
#include "remote.hpp"

#include <boost/signals2.hpp>

class Remote;

class Transaction {
public:
  typedef boost::signals2::signal<void ()> Signal;
  typedef Signal::slot_type Callback;

  Transaction(const Path &root);
  ~Transaction();

  void onReady(const Callback &callback) { m_onReady.connect(callback); }
  void onFinish(const Callback &callback) { m_onFinish.connect(callback); }

  void fetch(const RemoteList &);
  void fetch(const Remote &);

  void prepare();
  void run();
  void finish();

private:
  void install(Package *);
  void addError(const std::string &, const std::string &);

  DatabaseList m_databases;
  PackageList m_packages;

  DownloadQueue m_queue;

  Path m_root;
  Path m_dbPath;

  Signal m_onReady;
  Signal m_onFinish;
};

#endif
