/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2016  Christian Fillion
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

#include <queue>
#include <string>
#include <vector>

#include <boost/signals2.hpp>
#include <WDL/mutex.h>

#include <reaper_plugin.h>

class Download {
public:
  typedef std::queue<Download *> Queue;
  typedef boost::signals2::signal<void ()> Signal;
  typedef Signal::slot_type Callback;

  static void Init();
  static void Cleanup();

  Download(const std::string &name, const std::string &url);
  ~Download();

  const std::string &name() const { return m_name; }
  const std::string &url() const { return m_url; }
  int status() const { return m_status; }
  const std::string &contents() const { return m_contents; }

  bool isRunning();
  bool isAborted();

  void onStart(const Callback &callback) { m_onStart.connect(callback); }
  void onFinish(const Callback &callback) { m_onFinish.connect(callback); }
  void onDestroy(const Callback &callback) { m_onDestroy.connect(callback); }

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
  static DWORD WINAPI Worker(void *ptr);

  void finish(const int status, const std::string &contents);
  void finishInMainThread();
  void reset();

  std::string m_name;
  std::string m_url;

  WDL_Mutex m_mutex;

  HANDLE m_threadHandle;
  bool m_aborted;
  bool m_running;
  int m_status;
  std::string m_contents;

  Signal m_onStart;
  Signal m_onFinish;
  Signal m_onDestroy;
};

class DownloadQueue {
public:
  typedef boost::signals2::signal<void (Download *)> Signal;
  typedef Signal::slot_type Callback;

  DownloadQueue() {}
  DownloadQueue(const DownloadQueue &) = delete;
  ~DownloadQueue();

  void push(Download *);
  void start();
  void abort();

  bool idle() const { return m_queue.empty() && m_running.empty(); }

  void onPush(const Callback &callback) { m_onPush.connect(callback); }
  void onDone(const Callback &callback) { m_onDone.connect(callback); }

private:
  void clear();

  Download::Queue m_queue;
  std::vector<Download *> m_running;

  Signal m_onPush;
  Signal m_onDone;
};

#endif
