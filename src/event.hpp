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

#ifndef REAPACK_EVENT_HPP
#define REAPACK_EVENT_HPP

#include <map>
#include <memory>
#include <mutex>

typedef intptr_t Event;
class EventEmitter;

class EventLoop {
public:
  EventLoop();
  ~EventLoop();

  void push(EventEmitter *, Event);
  void forget(EventEmitter *);

private:
  static void mainThreadTimer();
  void processQueue();

  std::mutex m_mutex;
  std::multimap<EventEmitter *, Event> m_queue;
};

class EventEmitter {
public:
  EventEmitter();
  virtual ~EventEmitter();

  void emit(Event);

protected:
  friend EventLoop;
  virtual void eventHandler(Event) = 0;

private:
  std::shared_ptr<EventLoop> m_loop;
};

#endif
