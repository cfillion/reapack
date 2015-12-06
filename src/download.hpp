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
  void stop();

private:
  static std::vector<Download *> s_active;

  static void TimerTick();
  static size_t WriteData(char *, size_t, size_t, void *);
  static DWORD WINAPI Worker(void *ptr);

  void finish(const int status, const std::string &contents);
  void finishInMainThread();
  void abort();
  void reset();

  HANDLE m_threadHandle;
  bool m_aborted;
  bool m_finished;
  int m_status;
  std::string m_contents;

  std::string m_name;
  std::string m_url;

  Signal m_onStart;
  Signal m_onFinish;

  WDL_Mutex m_mutex;
};

class DownloadQueue {
public:
  typedef boost::signals2::signal<void (Download *)> Signal;
  typedef Signal::slot_type Callback;

  DownloadQueue() {}
  DownloadQueue(const DownloadQueue &) = delete;
  ~DownloadQueue();

  void push(Download *);

  size_t size() const { return m_queue.size(); }
  bool empty() const { return m_queue.empty(); }

  void onPush(const Callback &callback) { m_onPush.connect(callback); }

private:
  std::queue<Download *> m_queue;

  Signal m_onPush;
};

#endif
