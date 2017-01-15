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

#ifndef REAPACK_DOWNLOAD_HPP
#define REAPACK_DOWNLOAD_HPP

#include "config.hpp"

#include <functional>
#include <queue>
#include <string>
#include <vector>

#include <boost/signals2.hpp>
#include <WDL/mutex.h>

#include <reaper_plugin.h>

class Download {
public:
  typedef std::queue<Download *> Queue;
  typedef boost::signals2::signal<void ()> VoidSignal;
  typedef std::function<void ()> CleanupHandler;

  enum State {
    Idle,
    Running,
    Success,
    Failure,
    Aborted,
  };

  enum Flag {
    NoCacheFlag = 1<<0,
  };

  static void Init();
  static void Cleanup();

  Download(const std::string &name, const std::string &url,
    const NetworkOpts &, int flags = 0);
  ~Download();

  const std::string &name() const { return m_name; }
  const std::string &url() const { return m_url; }
  const NetworkOpts &options() const { return m_opts; }
  bool has(Flag f) const { return (m_flags & f) != 0; }

  State state();
  const std::string &contents();
  bool isAborted();
  short progress();

  void onStart(const VoidSignal::slot_type &slot) { m_onStart.connect(slot); }
  void onFinish(const VoidSignal::slot_type &slot) { m_onFinish.connect(slot); }
  void setCleanupHandler(const CleanupHandler &cb) { m_cleanupHandler = cb; }

  void start();
  void abort();
  void wait();

private:
  static WDL_Mutex s_mutex;
  static Queue s_finished;
  static size_t s_running;
  static void RegisterStart();
  static Download *NextFinished();
  static void MarkAsFinished(Download *);

  static void TimerTick();
  static size_t WriteData(char *, size_t, size_t, void *);
  static int UpdateProgress(void *, double, double, double, double);
  static DWORD WINAPI Worker(void *ptr);

  void setProgress(short);
  void finish(const State state, const std::string &contents);
  void finishInMainThread();
  void reset();

  std::string m_name;
  std::string m_url;
  NetworkOpts m_opts;
  int m_flags;

  WDL_Mutex m_mutex;

  HANDLE m_threadHandle;
  State m_state;
  bool m_aborted;
  std::string m_contents;
  short m_progress;

  VoidSignal m_onStart;
  VoidSignal m_onFinish;
  CleanupHandler m_cleanupHandler;
};

class DownloadQueue {
public:
  typedef boost::signals2::signal<void ()> VoidSignal;
  typedef boost::signals2::signal<void (Download *)> DownloadSignal;

  DownloadQueue() {}
  DownloadQueue(const DownloadQueue &) = delete;
  ~DownloadQueue();

  void push(Download *);
  void start();
  void abort();

  bool idle() const { return m_queue.empty() && m_running.empty(); }

  void onPush(const DownloadSignal::slot_type &slot) { m_onPush.connect(slot); }
  void onAbort(const VoidSignal::slot_type &slot) { m_onAbort.connect(slot); }
  void onDone(const VoidSignal::slot_type &slot) { m_onDone.connect(slot); }

private:
  void clear();

  Download::Queue m_queue;
  std::vector<Download *> m_running;

  DownloadSignal m_onPush;
  VoidSignal m_onAbort;
  VoidSignal m_onDone;
};

#endif
