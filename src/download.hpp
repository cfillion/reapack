#ifndef REAPACK_DOWNLOAD_HPP
#define REAPACK_DOWNLOAD_HPP

#include <functional>
#include <queue>
#include <string>
#include <vector>

#include <WDL/mutex.h>

#include <reaper_plugin.h>

typedef std::function<void(int, const std::string &)> DownloadCallback;

class Download {
public:
  enum StartCode {
    Started,
    AlreadyRunning,
    FileExists,
    WriteError,
  };

  Download(const std::string &url);
  ~Download();

  const std::string &url() const { return m_url; }

  bool isFinished();
  bool isAborted();

  void addCallback(const DownloadCallback &);
  StartCode start();
  void stop();

private:
  static std::vector<Download *> s_active;

  static void TimerTick();
  static size_t WriteData(char *, size_t, size_t, void *);
  static DWORD WINAPI Worker(void *ptr);

  void finish(const int status, const std::string &contents);
  void execCallbacks();
  void abort();
  void reset();

  HANDLE m_threadHandle;
  bool m_aborted;
  bool m_finished;
  int m_status;
  std::string m_contents;

  std::string m_url;
  std::vector<DownloadCallback> m_callback;

  WDL_Mutex m_mutex;
};

class DownloadQueue {
public:
  ~DownloadQueue();

  void push(const std::string &url, DownloadCallback);

private:
  std::queue<Download *> m_queue;
};

#endif
