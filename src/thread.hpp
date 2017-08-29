/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2017  Christian Fillion
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
#include <functional>
#include <queue>
#include <unordered_set>

#include <boost/signals2.hpp>
#include <WDL/mutex.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <swell-types.h>
#endif

class ThreadTask {
public:
  enum State {
    Idle,
    Running,
    Success,
    Failure,
    Aborted,
  };

  typedef boost::signals2::signal<void ()> VoidSignal;
  typedef std::function<void ()> CleanupHandler;

  ThreadTask();
  virtual ~ThreadTask();

  virtual bool concurrent() const = 0;

  void start(); // start a new thread
  void exec();  // runs in the current thread
  const String &summary() const { return m_summary; }
  void setState(State);
  State state() const { return m_state; }
  void setError(const ErrorInfo &err) { m_error = err; }
  const ErrorInfo &error() { return m_error; }

  void onStart(const VoidSignal::slot_type &slot) { m_onStart.connect(slot); }
  void onFinish(const VoidSignal::slot_type &slot) { m_onFinish.connect(slot); }
  void setCleanupHandler(const CleanupHandler &cb) { m_cleanupHandler = cb; }

  bool aborted() const { return m_abort; }
  void abort() { m_abort = true; }

protected:
  virtual bool run() = 0;

  void setSummary(const String &s) { m_summary = s; }

private:
  String m_summary;
  State m_state;
  ErrorInfo m_error;
  std::atomic_bool m_abort;

  VoidSignal m_onStart;
  VoidSignal m_onFinish;
  CleanupHandler m_cleanupHandler;
};

class WorkerThread {
public:
  WorkerThread();
  ~WorkerThread();

  void push(ThreadTask *);
  void clear();

private:
  static DWORD WINAPI run(void *);
  ThreadTask *nextTask();

  HANDLE m_wake;
  HANDLE m_thread;
  std::atomic_bool m_exit;
  WDL_Mutex m_mutex;
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

  WDL_Mutex m_mutex;
  size_t m_active;
  std::queue<Notification> m_queue;
};

#endif
