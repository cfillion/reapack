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

#include "thread.hpp"

#include "download.hpp"

#include <reaper_plugin_functions.h>

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
  m_state = state;

  switch(state) {
  case Idle:
    break;
  case Running:
    m_onStart();
    break;
  case Success:
  case Failure:
  case Aborted:
    m_onFinish();
    m_cleanupHandler();
    break;
  }
}

WorkerThread::WorkerThread() : m_exit(false)
{
  m_wake = CreateEvent(nullptr, true, false, AUTO_STR("WakeEvent"));
  m_thread = CreateThread(nullptr, 0, run, (void *)this, 0, nullptr);
}

WorkerThread::~WorkerThread()
{
  // remove all pending tasks then wake the thread to make it exit
  m_exit = true;
  SetEvent(m_wake);

  WaitForSingleObject(m_thread, INFINITE);

  CloseHandle(m_wake);
  CloseHandle(m_thread);
}

DWORD WINAPI WorkerThread::run(void *ptr)
{
  WorkerThread *thread = static_cast<WorkerThread *>(ptr);

  void *context = Download::ContextInit();

  while(!thread->m_exit) {
    while(ThreadTask *task = thread->nextTask())
      task->run(context);

    ResetEvent(thread->m_wake);
    WaitForSingleObject(thread->m_wake, INFINITE);
  }

  Download::ContextCleanup(context);

  return 0;
}

ThreadTask *WorkerThread::nextTask()
{
  WDL_MutexLock lock(&m_mutex);

  if(m_queue.empty())
    return nullptr;

  ThreadTask *task = m_queue.front();
  m_queue.pop();
  return task;
}

void WorkerThread::push(ThreadTask *task)
{
  WDL_MutexLock lock(&m_mutex);

  m_queue.push(task);
  SetEvent(m_wake);
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

    // call m_onDone() only after every onFinish slots ran
    if(m_running.empty())
      m_onDone();
  });

  task->setCleanupHandler([=] { delete task; });

  auto &thread = m_pool[m_running.size() % m_pool.size()];
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
  WDL_MutexLock lock(&m_mutex);

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
  WDL_MutexLock lock(&m_mutex);

  while(!m_queue.empty()) {
    const Notification &notif = m_queue.front();
    notif.first->setState(notif.second);
    m_queue.pop();
  }
}
