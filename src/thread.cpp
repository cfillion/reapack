/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2023  Christian Fillion
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

#include "thread.hpp"

#include <reaper_plugin_functions.h>

#ifdef _WIN32
  typedef void (__stdcall *_tls_callback_type)(HANDLE, DWORD const dwReason, LPVOID);
  extern "C" extern const _tls_callback_type __dyn_tls_dtor_callback;
#endif

ThreadTask::ThreadTask() : m_state(Idle), m_abort(false)
{
}

ThreadTask::~ThreadTask()
{
}

void ThreadTask::exec()
{
  State state = Idle;

  if(!aborted()) {
    onStartAsync();
    state = run() ? Success : Failure;
  }

  if(aborted()) // may have changed while the task was running
    state = Aborted;

  m_state = state;
  onFinishAsync();
}

WorkerThread::WorkerThread() : m_stop(false), m_thread(&WorkerThread::run, this)
{
}

WorkerThread::~WorkerThread()
{
  {
    std::lock_guard lock(m_mutex);
    m_stop = true;
  }

  m_wake.notify_one();
  m_thread.join();
}

void WorkerThread::run()
{
  std::unique_lock<std::mutex> lock(m_mutex);

  while(true) {
    m_wake.wait(lock, [=] { return !m_queue.empty() || m_stop; });

    if(m_stop)
      break;

    ThreadTask *task = m_queue.front();
    m_queue.pop();

    lock.unlock();
    task->exec();
    lock.lock();
  }

#ifdef _WIN32
  // HACK: Destruct thread-local storage objects earlier on Windows to avoid a
  // possible deadlock when tearing down the cURL context with active HTTPS
  // connections on some computers [p=2038163]. InitializeSecurityContext would
  // hang forever waiting for a semaphore for undetermined reasons...
  //
  // Note that the destructors are not called a second time when this function
  // is invoked by the C++ runtime during the normal thread shutdown procedure.
  __dyn_tls_dtor_callback(nullptr, DLL_THREAD_DETACH, nullptr);
#endif
}

ThreadTask *WorkerThread::nextTask()
{
  std::lock_guard<std::mutex> guard(m_mutex);

  if(m_queue.empty())
    return nullptr;

  ThreadTask *task = m_queue.front();
  m_queue.pop();
  return task;
}

void WorkerThread::push(ThreadTask *task)
{
  std::lock_guard<std::mutex> guard(m_mutex);

  m_queue.push(task);
  m_wake.notify_one();
}

ThreadPool::~ThreadPool()
{
  // don't emit ThreadPool::onAbort from the destructor
  // which is most likely to cause a crash
  onAbort.reset();

  abort();
}

void ThreadPool::push(ThreadTask *task)
{
  onPush(task);
  m_running.insert(task);

  task->onFinishAsync >> [=] {
    m_running.erase(task);

    // 'this' (captured variable) isn't valid after the delete below
    ThreadPool *self = this;
    delete task;

    // call m_onDone() only after every onFinish slots ran
    if(self->m_running.empty())
      self->onDone();
  };

  const size_t nextThread = m_running.size() % m_pool.size();
  auto &thread = task->concurrent() ? m_pool[nextThread] : m_pool.front();
  if(!thread)
    thread = std::make_unique<WorkerThread>();

  thread->push(task);
}

void ThreadPool::abort()
{
  for(ThreadTask *task : m_running)
    task->abort();

  onAbort();
}
