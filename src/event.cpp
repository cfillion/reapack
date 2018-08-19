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

static std::weak_ptr<EventLoop> s_loop;

EventLoop::EventLoop()
{
  plugin_register("timer", reinterpret_cast<void *>(&mainThreadTimer));
}

EventLoop::~EventLoop()
{
  plugin_register("-timer", reinterpret_cast<void *>(&mainThreadTimer));
}

void EventLoop::mainThreadTimer()
{
  s_loop.lock()->processQueue();
}

void EventLoop::push(EventEmitter *emitter, const Event event)
{
  std::lock_guard<std::mutex> guard(m_mutex);
  m_queue.insert({emitter, event});
}

void EventLoop::forget(EventEmitter *emitter)
{
  std::lock_guard<std::mutex> guard(m_mutex);
  m_queue.erase(emitter);
}

void EventLoop::processQueue()
{
  decltype(m_queue) events;

  {
    std::lock_guard<std::mutex> guard(m_mutex);
    std::swap(events, m_queue);
  }

  for(const auto &[emitter, event] : events)
    emitter->eventHandler(event);
}

EventEmitter::EventEmitter()
{
  if(s_loop.expired())
    s_loop = m_loop = std::make_shared<EventLoop>();
  else
    m_loop = s_loop.lock();
}

EventEmitter::~EventEmitter()
{
  if(s_loop.use_count() > 1)
    m_loop->forget(this);
}

void EventEmitter::emit(const Event event)
{
  m_loop->push(this, event);
}
