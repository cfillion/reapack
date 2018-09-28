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
#include "event.hpp"

#include <array>
#include <condition_variable>
#include <functional>
#include <queue>
#include <thread>
#include <unordered_set>

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

  ThreadTask();
  virtual ~ThreadTask();

  virtual bool concurrent() const = 0;

  void exec();  // runs in the current thread
  const std::string &summary() const { return m_summary; }
  State state() const { return m_state; }
  void setError(const ErrorInfo &err) { m_error = err; }
  const ErrorInfo &error() { return m_error; }

  bool aborted() const { return m_abort; }
  void abort() { m_abort = true; }

  AsyncEvent<void()> onStartAsync;
  AsyncEvent<void()> onFinishAsync;

protected:
  virtual bool run() = 0;

  void setSummary(const std::string &s) { m_summary = s; }

private:
  std::string m_summary;
  State m_state;
  ErrorInfo m_error;
  std::atomic_bool m_abort;
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

  bool m_stop;
  std::mutex m_mutex;
  std::condition_variable m_wake;
  std::queue<ThreadTask *> m_queue;

  std::thread m_thread;
};

class ThreadPool {
public:
  ThreadPool() {}
  ThreadPool(const ThreadPool &) = delete;
  ~ThreadPool();

  void push(ThreadTask *);
  void abort();

  bool idle() const { return m_running.empty(); }

  Event<void(ThreadTask *)> onPush;
  Event<void()> onAbort;
  Event<void()> onDone;

private:
  std::array<std::unique_ptr<WorkerThread>, 3> m_pool;
  std::unordered_set<ThreadTask *> m_running;
};

#endif
