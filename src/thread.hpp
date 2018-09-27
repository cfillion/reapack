#ifndef REAPACK_THREAD_HPP
#define REAPACK_THREAD_HPP

#include "event.hpp"

#include <array>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <queue>
#include <thread>

class ThreadPool;

class WorkUnit {
public:
  WorkUnit() : m_aborted(false) {}
  virtual ~WorkUnit() = default;

  Event<void()> onStart, onFinish;

  virtual bool run() = 0;

  void abort() { m_aborted = true; }
  bool aborted() { return m_aborted; }

private:
  std::atomic_bool m_aborted;
};

class WorkerThread {
public:

  WorkerThread(ThreadPool *);
  ~WorkerThread();

  void push(const std::shared_ptr<WorkUnit> &);
  void abort();
  bool idling();

private:
  void run();

  ThreadPool *m_pool;

  std::mutex m_mutex;
  bool m_stop;
  WorkUnit *m_current;
  std::condition_variable m_wake;
  std::queue<std::shared_ptr<WorkUnit>> m_queue;

  std::thread m_thread;
};

class ThreadPool {
public:
  ThreadPool();
  ThreadPool(const ThreadPool &) = delete;

  void push(const std::shared_ptr<WorkUnit> &);

  bool wait();
  void abort();

  void threadIdle(WorkerThread *);

  Event<void(bool)> onCancellableChanged;

private:
  size_t m_nextThread;
  std::atomic_bool m_aborted;
  std::mutex m_mutex;
  std::condition_variable m_cv;
  std::array<std::unique_ptr<WorkerThread>, 3> m_workers;
};

#endif
