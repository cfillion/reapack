#ifndef REAPACK_DOWNLOAD_HPP
#define REAPACK_DOWNLOAD_HPP

#include <functional>
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

  Download(const char *url);
  ~Download();

  const char *url() const { return m_url; }

  bool isFinished();
  bool isAborted();
  int status();
  const std::string &contents();

  void addCallback(const DownloadCallback &);
  StartCode start();
  void stop();

private:
  static std::vector<Download *> s_active;

  static void timerTick();
  static DWORD WINAPI worker(void *ptr);

  void finish(const int status, const std::string &contents);
  void execCallbacks();
  void abort();

  HANDLE m_threadHandle;
  bool m_aborted;
  bool m_finished;
  int m_status;
  std::string m_contents;

  const char *m_url;
  std::vector<DownloadCallback> m_callback;

  WDL_Mutex m_mutex;
};

#endif
