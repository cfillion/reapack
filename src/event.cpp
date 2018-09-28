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

#include "event.hpp"

#include <reaper_plugin_functions.h>

static std::weak_ptr<AsyncEventImpl::Loop> s_loop;

AsyncEventImpl::Loop::Loop()
{
  plugin_register("timer", reinterpret_cast<void *>(&mainThreadTimer));
}

AsyncEventImpl::Loop::~Loop()
{
  plugin_register("-timer", reinterpret_cast<void *>(&mainThreadTimer));
}

void AsyncEventImpl::Loop::mainThreadTimer()
{
  s_loop.lock()->processQueue();
}

void AsyncEventImpl::Loop::push(const MainThreadFunc &event, const void *source)
{
  std::lock_guard<std::mutex> guard(m_mutex);
  m_queue.insert({source, event});
}

void AsyncEventImpl::Loop::forget(const void *source)
{
  std::lock_guard<std::mutex> guard(m_mutex);
  m_queue.erase(source);
}

void AsyncEventImpl::Loop::processQueue()
{
  decltype(m_queue) events;

  {
    std::lock_guard<std::mutex> guard(m_mutex);
    std::swap(events, m_queue);
  }

  for(const auto &[emitter, func] : events)
    func();
}

AsyncEventImpl::Emitter::Emitter()
{
  if(s_loop.expired())
    s_loop = m_loop = std::make_shared<Loop>();
  else
    m_loop = s_loop.lock();
}

AsyncEventImpl::Emitter::~Emitter()
{
  if(s_loop.use_count() > 1)
    m_loop->forget(this);
}

void AsyncEventImpl::Emitter::runInMainThread(const MainThreadFunc &event) const
{
  m_loop->push(event, this);
}
