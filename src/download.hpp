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

  Download(const std::string &name, const std::string &url);
  ~Download();

  const std::string &name() const { return m_name; }
  const std::string &url() const { return m_url; }
  int status() const { return m_status; }
  const std::string &contents() const { return m_contents; }

  bool isFinished();
  bool isAborted();

  void onStart(const Callback &callback) { m_onStart.connect(callback); }
  void onFinish(const Callback &callback) { m_onFinish.connect(callback); }

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
  bool m_finished;
  int m_status;
  std::string m_contents;

  Signal m_onStart;
  Signal m_onFinish;
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

private:
  void clear();

  Download::Queue m_queue;
  std::vector<Download *> m_running;

  Signal m_onPush;
};

#endif
