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

#include <array>
#include <atomic>
#include <functional>
#include <queue>
#include <string>
#include <unordered_set>

#include <boost/signals2.hpp>
#include <WDL/mutex.h>

#include <curl/curl.h>
#include <reaper_plugin.h>

class Download {
public:
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

  void setState(State);
  State state() const { return m_state; }
  const std::string &contents() { return m_contents; }
  bool isAborted() { return m_aborted; }

  void onStart(const VoidSignal::slot_type &slot) { m_onStart.connect(slot); }
  void onFinish(const VoidSignal::slot_type &slot) { m_onFinish.connect(slot); }
  void setCleanupHandler(const CleanupHandler &cb) { m_cleanupHandler = cb; }

  void start();
  void abort() { m_aborted = true; }

  void exec(CURL *);

private:
  static size_t WriteData(char *, size_t, size_t, void *);
  static int UpdateProgress(void *, double, double, double, double);

  void finish(const State state, const std::string &contents);
  void reset();

  std::string m_name;
  std::string m_url;
  NetworkOpts m_opts;
  int m_flags;

  State m_state;
  std::atomic_bool m_aborted;
  std::string m_contents;

  VoidSignal m_onStart;
  VoidSignal m_onFinish;
  CleanupHandler m_cleanupHandler;
};

class DownloadThread {
public:
  DownloadThread();
  ~DownloadThread();

  void push(Download *);
  void clear();

private:
  static DWORD WINAPI thread(void *);
  Download *nextDownload();

  HANDLE m_wake;
  HANDLE m_thread;
  std::atomic_bool m_exit;
  WDL_Mutex m_mutex;
  std::queue<Download *> m_queue;
};

class DownloadQueue {
public:
  typedef boost::signals2::signal<void ()> VoidSignal;
  typedef boost::signals2::signal<void (Download *)> DownloadSignal;

  DownloadQueue() {}
  DownloadQueue(const DownloadQueue &) = delete;
  ~DownloadQueue();

  void push(Download *);
  void abort();

  bool idle() const { return m_running.empty(); }

  void onPush(const DownloadSignal::slot_type &slot) { m_onPush.connect(slot); }
  void onAbort(const VoidSignal::slot_type &slot) { m_onAbort.connect(slot); }
  void onDone(const VoidSignal::slot_type &slot) { m_onDone.connect(slot); }

private:
  std::array<std::unique_ptr<DownloadThread>, 3> m_pool;
  std::unordered_set<Download *> m_running;

  DownloadSignal m_onPush;
  VoidSignal m_onAbort;
  VoidSignal m_onDone;
};

// This singleton class receives state change notifications from a
// worker thread and applies them in the main thread
class DownloadNotifier {
  typedef std::pair<Download *, Download::State> Notification;

public:
  static DownloadNotifier *get();

  void start();
  void stop();

  void notify(const Notification &);

private:
  static DownloadNotifier *s_instance;
  static void tick();

  DownloadNotifier() : m_active(0) {}
  ~DownloadNotifier() = default;
  void processQueue();

  WDL_Mutex m_mutex;
  size_t m_active;
  std::queue<Notification> m_queue;
};

#endif
