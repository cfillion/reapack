#ifndef REAPACK_TRANSACTION_HPP
#define REAPACK_TRANSACTION_HPP

#include "database.hpp"
#include "download.hpp"
#include "path.hpp"
#include "registry.hpp"
#include "remote.hpp"

#include <boost/signals2.hpp>

class Transaction {
public:
  typedef boost::signals2::signal<void ()> Signal;
  typedef Signal::slot_type Callback;

  Transaction(Registry *reg, const Path &root);
  ~Transaction();

  void onReady(const Callback &callback) { m_onReady.connect(callback); }
  void onFinish(const Callback &callback) { m_onFinish.connect(callback); }

  void fetch(const RemoteMap &);
  void fetch(const Remote &);

  void run();
  void cancel();

  const PackageList &packages() const { return m_packages; }
  DownloadQueue *downloadQueue() { return &m_queue; }

private:
  void prepare();
  void finish();

  void install(Package *);
  void addError(const std::string &msg, const std::string &title);

  Registry *m_registry;

  DatabaseList m_databases;
  PackageList m_packages;

  DownloadQueue m_queue;

  Path m_root;
  Path m_dbPath;

  Signal m_onReady;
  Signal m_onFinish;
};

#endif
