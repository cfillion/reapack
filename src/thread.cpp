#include "thread.hpp"

#include <reaper_plugin_functions.h>

#ifdef _WIN32
  typedef void (*_tls_callback_type)(HANDLE, DWORD const dwReason, LPVOID);
  extern "C" extern const _tls_callback_type __dyn_tls_dtor_callback;
#endif

WorkerThread::WorkerThread(ThreadPool *parent)
  : m_pool(parent), m_stop(false), m_current(nullptr),
  m_thread(&WorkerThread::run, this)
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
    if(m_queue.empty())
      m_pool->threadIdle(this);

    m_wake.wait(lock, [=] { return !m_queue.empty() || m_stop; });

    if(m_stop)
      break;

    std::shared_ptr<WorkUnit> work = m_queue.front();
    m_queue.pop();
    m_current = work.get();

    lock.unlock();
    work->onStart();
    work->run();
    work->onFinish();
    lock.lock();

    m_current = nullptr;
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

void WorkerThread::push(const std::shared_ptr<WorkUnit> &work)
{
  std::lock_guard<std::mutex> lock(m_mutex);

  m_queue.push(work);
  m_wake.notify_one();
}

void WorkerThread::abort()
{
  std::lock_guard<std::mutex> lock(m_mutex);

  if(m_current)
    m_current->abort();

  decltype(m_queue){}.swap(m_queue); // clear the queue
}

bool WorkerThread::idling()
{
  std::lock_guard<std::mutex> guard(m_mutex);

  return !m_current && m_queue.empty();
}

ThreadPool::ThreadPool() : m_nextThread(0), m_aborted(false)
{
}

void ThreadPool::push(const std::shared_ptr<WorkUnit> &work)
{
  const size_t id = m_nextThread;
  m_nextThread = (m_nextThread + 1) % m_workers.size();

  auto &thread = m_workers[id];

  if(!thread)
    thread = std::make_unique<WorkerThread>(this);

  thread->push(work);
}

bool ThreadPool::wait()
{
  std::unique_lock<std::mutex> lock(m_mutex);

  onCancellableChanged(true);

  m_cv.wait(lock, [=] {
    for(auto &thread : m_workers) {
      if(thread && !thread->idling())
        return false;
    }

    return true;
  });

  onCancellableChanged(false);

  return !m_aborted;
}

void ThreadPool::threadIdle(WorkerThread *)
{
  m_cv.notify_all();
}

void ThreadPool::abort()
{
  m_aborted = true;

  for(auto &thread : m_workers) {
    if(thread)
      thread->abort();
  }
}
