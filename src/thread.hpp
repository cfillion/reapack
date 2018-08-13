/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2018  Christian Fillion
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef REAPACK_THREAD_HPP
#define REAPACK_THREAD_HPP

#include "errors.hpp"

#include <array>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <queue>
#include <thread>
#include <unordered_set>

#include <boost/signals2.hpp>

class ThreadTask {
public:
  enum State {
    Idle,
    Queued,
    Running,
    Success,
    Failure,
    Aborted,
  };

  typedef boost::signals2::signal<void ()> VoidSignal;

  ThreadTask();
  virtual ~ThreadTask();

  virtual bool concurrent() const = 0;

  void start(); // start a new thread
  void exec();  // runs in the current thread
  const std::string &summary() const { return m_summary; }
  void setState(State);
  State state() const { return m_state; }
  void setError(const ErrorInfo &err) { m_error = err; }
  const ErrorInfo &error() { return m_error; }

  void onStart(const VoidSignal::slot_type &slot) { m_onStart.connect(slot); }
  void onFinish(const VoidSignal::slot_type &slot);

  bool aborted() const { return m_abort; }
  void abort() { m_abort = true; }

protected:
  virtual bool run() = 0;

  void setSummary(const std::string &s) { m_summary = s; }

private:
  std::string m_summary;
  State m_state;
  ErrorInfo m_error;
  std::atomic_bool m_abort;

  VoidSignal m_onStart;
  VoidSignal m_onFinish;
};

class WorkerThread {
public:
  WorkerThread();
  ~WorkerThread();

  void push(ThreadTask *);
  void clear();

private:
  void run();
  ThreadTask *nextTask();

  std::thread m_thread;
  std::condition_variable m_wake;
  std::mutex m_mutex;
  std::atomic_bool m_exit;
  std::queue<ThreadTask *> m_queue;
};

class ThreadPool {
public:
  typedef boost::signals2::signal<void ()> VoidSignal;
  typedef boost::signals2::signal<void (ThreadTask *)> TaskSignal;

  ThreadPool() {}
  ThreadPool(const ThreadPool &) = delete;
  ~ThreadPool();

  void push(ThreadTask *);
  void abort();

  bool idle() const { return m_running.empty(); }

  void onPush(const TaskSignal::slot_type &slot) { m_onPush.connect(slot); }
  void onAbort(const VoidSignal::slot_type &slot) { m_onAbort.connect(slot); }
  void onDone(const VoidSignal::slot_type &slot) { m_onDone.connect(slot); }

private:
  std::array<std::unique_ptr<WorkerThread>, 3> m_pool;
  std::unordered_set<ThreadTask *> m_running;

  TaskSignal m_onPush;
  VoidSignal m_onAbort;
  VoidSignal m_onDone;
};

// This singleton class receives state change notifications from a
// worker thread and applies them in the main thread
class ThreadNotifier {
  typedef std::pair<ThreadTask *, ThreadTask::State> Notification;

public:
  static ThreadNotifier *get();

  void start();
  void stop();

  void notify(const Notification &);

private:
  static ThreadNotifier *s_instance;
  static void tick();

  ThreadNotifier() : m_active(0) {}
  ~ThreadNotifier() = default;
  void processQueue();

  std::mutex m_mutex;
  size_t m_active;
  std::queue<Notification> m_queue;
};

#endif
