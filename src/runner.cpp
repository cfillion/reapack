#include "runner.hpp"

#include "errors.hpp"
#include "registry.hpp"
#include "thread.hpp"
#include "transaction.hpp"

Runner::Runner() : m_stop(false)
{
  m_pool.onCancellableChanged >> std::ref(onSetCancellableAsync);
}

Runner::~Runner()
{
  printf("destructing runner\n");
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stop = true;
  }

  m_wake.notify_one();
  m_thread.join();
}

void Runner::push(std::unique_ptr<Transaction> &tx)
{
  std::lock_guard<std::mutex> lock(m_mutex);
  m_queue.push(move(tx));
  m_wake.notify_one();

  if(!m_thread.joinable())
    m_thread = std::thread{&Runner::run, this};
}

void Runner::abort()
{
  std::lock_guard<std::mutex> lock(m_mutex);
  decltype(m_queue)().swap(m_queue);
  m_pool.abort();
}

std::unique_ptr<Registry> Runner::loadRegistry()
{
  try {
    return std::make_unique<Registry>(Path::REGISTRY.prependRoot());
  }
  catch(const reapack_error &) {
    // TODO: report error
    return nullptr;
  }
}

void Runner::run()
{
  printf("Runner::run()\n");
  auto registry = loadRegistry();

  if(!registry)
    return;

  std::unique_lock<std::mutex> lock(m_mutex);

  while(true) {
    m_wake.wait(lock, [=] { return !m_queue.empty() || m_stop; });

    if(m_stop)
      break;

    std::unique_ptr<Transaction> transaction = move(m_queue.front());
    m_queue.pop();

    lock.unlock();
    printf("running transaction %p\n", transaction.get());
    transaction->run(registry.get(), &m_pool);
    lock.lock();

    if(m_queue.empty())
      onFinishAsync();
  }

  printf("runner is over!\n");
}

// bool Runner::test()
// {
//   bool allOk = true;
//
//   m_registry->savepoint();

  // for(const TxTaskPtr &task : m_queue) {
  //   if(!task->test())
  //     allOk = false;
  // }

//   m_registry->restore();
//
//   return allOk;
// }
