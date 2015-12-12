#ifndef REAPACK_TRANSACTION_HPP
#define REAPACK_TRANSACTION_HPP

#include "database.hpp"
#include "download.hpp"
#include "path.hpp"
#include "registry.hpp"
#include "remote.hpp"

#include <boost/signals2.hpp>

class PackageTransaction;

class Transaction {
public:
  typedef boost::signals2::signal<void ()> Signal;
  typedef Signal::slot_type Callback;

  typedef std::pair<Package *, const Registry::QueryResult> PackageEntry;
  typedef std::vector<PackageEntry> PackageEntryList;

  struct Error {
    std::string message;
    std::string title;
  };

  typedef std::vector<const Error> ErrorList;

  Transaction(Registry *reg, const Path &root);
  ~Transaction();

  void onReady(const Callback &callback) { m_onReady.connect(callback); }
  void onFinish(const Callback &callback) { m_onFinish.connect(callback); }

  void fetch(const RemoteMap &);
  void fetch(const Remote &);
  void run();
  void cancel();

  bool isCancelled() const { return m_isCancelled; }
  DownloadQueue *downloadQueue() { return &m_queue; }
  const PackageEntryList &packages() const { return m_packages; }
  const PackageEntryList &newPackages() const { return m_new; }
  const PackageEntryList &updates() const { return m_updates; }
  const ErrorList &errors() const { return m_errors; }

private:
  friend PackageTransaction;

  void prepare();
  void finish();

  void saveDatabase(Download *);
  bool saveFile(Download *, const Path &);
  void addError(const std::string &msg, const std::string &title);

  Registry *m_registry;

  Path m_root;
  Path m_dbPath;
  bool m_isCancelled;

  DatabaseList m_databases;
  DownloadQueue m_queue;
  PackageEntryList m_packages;
  PackageEntryList m_new;
  PackageEntryList m_updates;
  ErrorList m_errors;

  std::vector<PackageTransaction *> m_transactions;

  Signal m_onReady;
  Signal m_onFinish;
};

class PackageTransaction {
public:
  static bool isInstalled(Version *, const Path &root);

  typedef boost::signals2::signal<void ()> Signal;
  typedef Signal::slot_type Callback;

  PackageTransaction(Transaction *parent);

  void onFinish(const Callback &callback) { m_onFinish.connect(callback); }

  void install(Version *ver);
  void abort();

private:
  void saveSource(Download *, Source *);
  Path installPath(Source *) const;

  void finish();

  Transaction *m_transaction;
  std::vector<Download *> m_remaining;

  Signal m_onFinish;
};

#endif
