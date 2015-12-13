#ifndef REAPACK_PKG_TRANSACTION_HPP
#define REAPACK_PKG_TRANSACTION_HPP

#include <boost/signals2.hpp>
#include <vector>

class Download;
class Source;
class Transaction;
class Path;
class Version;

class PackageTransaction {
public:
  static bool isInstalled(Version *, Transaction *);

  typedef boost::signals2::signal<void ()> Signal;
  typedef Signal::slot_type Callback;

  PackageTransaction(Transaction *parent);

  void onCommit(const Callback &callback) { m_onCommit.connect(callback); }
  void onFinish(const Callback &callback) { m_onFinish.connect(callback); }

  void install(Version *ver);
  void commit();
  void cancel();

private:
  typedef std::pair<Path, Path> PathPair;

  void finish();
  void rollback();

  void saveSource(Download *, Source *);

  Transaction *m_transaction;
  bool m_isCancelled;
  std::vector<Download *> m_remaining;
  std::vector<PathPair> m_files;

  Signal m_onCommit;
  Signal m_onFinish;
};

#endif
