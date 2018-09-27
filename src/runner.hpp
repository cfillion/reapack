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

#ifndef REAPACK_RUNNER_HPP
#define REAPACK_RUNNER_HPP

#include "event.hpp"
#include "thread.hpp"

#include <condition_variable>
#include <queue>
#include <thread>

class Registry;
class Transaction;

class Runner {
public:
  Runner();
  ~Runner();

  void push(std::unique_ptr<Transaction> &);
  void abort();

  AsyncEvent<void()> onFinishAsync;
  AsyncEvent<void(bool)> onSetCancellableAsync;

private:
  std::unique_ptr<Registry> loadRegistry();
  void run();

  std::mutex m_mutex;
  bool m_start;
  bool m_stop;
  std::queue<std::unique_ptr<Transaction>> m_queue;
  std::condition_variable m_wake;

  ThreadPool m_pool;
  std::thread m_thread;
};

#endif
