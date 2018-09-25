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

#include "thread.hpp"

#include <reaper_plugin_functions.h>

#ifdef _WIN32
  typedef void (*_tls_callback_type)(HANDLE, DWORD const dwReason, LPVOID);
  extern "C" extern const _tls_callback_type __dyn_tls_dtor_callback;
#endif

using namespace std;

ThreadNotifier *ThreadNotifier::s_instance = nullptr;

ThreadTask::ThreadTask()
  : m_state(Idle), m_abort(false)
{
  ThreadNotifier::get()->start();
}

ThreadTask::~ThreadTask()
{
  ThreadNotifier::get()->stop();
}

void ThreadTask::setState(const State state)
{
  // The task may have been aborted while the task was running or just before
  // the finish notification got received in the main thread.
  m_state = aborted() ? Aborted : state;

  switch(state) {
  case Idle:
  case Queued:
    break;
  case Running:
    m_onStart();
    break;
  case Success:
  case Failure:
  case Aborted:
    m_onFinish();
    break;
  }
}

void ThreadTask::start()
{
  WorkerThread *thread = new WorkerThread;
  thread->push(this);
  onFinish([thread] { delete thread; });
}

void ThreadTask::onFinish(const VoidSignal::slot_type &slot)
{
  // The task has a slot deleting itself at this point, accepting
  // any more slots at this point is a very bad idea.
  assert(m_state < Queued);

  m_onFinish.connect(slot);
}

void ThreadTask::exec()
{
  State state = Aborted;

  if(!aborted()) {
    ThreadNotifier::get()->notify({this, Running});
    state = run() ? Success : Failure;
  }

  ThreadNotifier::get()->notify({this, state});
};

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
  unique_lock<mutex> lock(m_mutex);

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
  lock_guard<mutex> guard(m_mutex);

  if(m_queue.empty())
    return nullptr;

  ThreadTask *task = m_queue.front();
  m_queue.pop();
  return task;
}

void WorkerThread::push(ThreadTask *task)
{
  lock_guard<mutex> guard(m_mutex);

  task->setState(ThreadTask::Queued);
  m_queue.push(task);
  m_wake.notify_one();
}

ThreadPool::~ThreadPool()
{
  // don't emit ThreadPool::onAbort from the destructor
  // which is most likely to cause a crash
  m_onAbort.disconnect_all_slots();

  abort();
}

void ThreadPool::push(ThreadTask *task)
{
  m_onPush(task);
  m_running.insert(task);

  task->onFinish([=] {
    m_running.erase(task);

    delete task;

    // call m_onDone() only after every onFinish slots ran
    if(m_running.empty())
      m_onDone();
  });

  const size_t nextThread = m_running.size() % m_pool.size();
  auto &thread = task->concurrent() ? m_pool[nextThread] : m_pool.front();
  if(!thread)
    thread = make_unique<WorkerThread>();

  thread->push(task);
}

void ThreadPool::abort()
{
  for(ThreadTask *task : m_running)
    task->abort();

  m_onAbort();
}

ThreadNotifier *ThreadNotifier::get()
{
  if(!s_instance)
    s_instance = new ThreadNotifier;

  return s_instance;
}

void ThreadNotifier::start()
{
  if(!m_active++)
    plugin_register("timer", (void *)tick);
}

void ThreadNotifier::stop()
{
  --m_active;
}

void ThreadNotifier::notify(const Notification &notif)
{
  lock_guard<mutex> guard(m_mutex);

  m_queue.push(notif);
}

void ThreadNotifier::tick()
{
  ThreadNotifier *instance = ThreadNotifier::get();
  instance->processQueue();

  // doing this in stop() would cause a use after free of m_mutex in processQueue
  if(!instance->m_active) {
    plugin_register("-timer", (void *)tick);

    delete s_instance;
    s_instance = nullptr;
  }
}

void ThreadNotifier::processQueue()
{
  lock_guard<mutex> guard(m_mutex);

  while(!m_queue.empty()) {
    const auto &[task, state] = m_queue.front();
    task->setState(state);
    m_queue.pop();
  }
}
