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

#include <functional>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

template<class T>
class Event;

template<class R, class... Args>
class Event<R(Args...)> {
public:
  typedef std::function<R(Args...)> Handler;
  typedef typename std::conditional<
    std::is_void<R>::value, void, std::optional<R>>::type ReturnType;

  Event() = default;
  Event(const Event &) = delete;

  operator bool() const { return !m_handlers.empty(); }
  void reset() { m_handlers.clear(); }

  Event<R(Args...)> &operator>>(const Handler &func)
  {
    m_handlers.push_back(func);
    return *this;
  }

  ReturnType operator()(Args... args) const
  {
    if constexpr (std::is_void<R>::value) {
      for(const auto &func : m_handlers)
        func(std::forward<Args>(args)...);
    }
    else {
      ReturnType ret;
      for(const auto &func : m_handlers)
        ret = func(std::forward<Args>(args)...);
      return ret;
    }
  }

private:
  std::vector<Handler> m_handlers;
};

namespace AsyncEventImpl {
  typedef std::function<void ()> MainThreadFunc;

  class Loop {
  public:
    Loop();
    ~Loop();

    void push(const MainThreadFunc &, const void *source = nullptr);
    void forget(const void *source);

  private:
    static void mainThreadTimer();
    void processQueue();

    std::mutex m_mutex;
    std::multimap<const void *, MainThreadFunc> m_queue;
  };

  class Emitter {
  public:
    Emitter();
    ~Emitter();

    void runInMainThread(const MainThreadFunc &) const;

  private:
    std::shared_ptr<Loop> m_loop;
  };
};

template<class T>
class AsyncEvent;

template<class R, class... Args>
class AsyncEvent<R(Args...)> : public Event<R(Args...)> {
public:
  using typename Event<R(Args...)>::ReturnType;

  std::future<ReturnType> operator()(Args... args) const
  {
    auto promise = std::make_shared<std::promise<ReturnType>>();

    // don't wait until the next timer tick to return nothing if there are no
    // handlers currently subscribed to the event
    if(!*this) {
      if constexpr (std::is_void<R>::value)
        promise->set_value();
      else
        promise->set_value(std::nullopt);

      return promise->get_future();
    }

    m_emitter.runInMainThread([=] {
      if constexpr (std::is_void<R>::value) {
        Event<R(Args...)>::operator()(args...);
        promise->set_value();
      }
      else
        promise->set_value(Event<R(Args...)>::operator()(args...));
    });

    return promise->get_future();
  }

private:
  AsyncEventImpl::Emitter m_emitter;
};

#endif
