#include "download.hpp"

#include <cstdlib>

#include <WDL/jnetlib/httpget.h>
#include <WDL/jnetlib/jnetlib.h>

#include <reaper_plugin_functions.h>

std::vector<Download *> Download::s_active;

static const int DOWNLOAD_TIMEOUT = 60 * 10;

Download::Download(const char *url)
  : m_threadHandle(0), m_stop(false), m_status(0)
{
  m_url = url;
}

Download::~Download()
{
  stop();
}

void Download::addCallback(const DownloadCallback &callback)
{
  m_callback.push_back(callback);
}

void Download::timerTick() // static
{
  std::vector<Download *> &activeDownloads = Download::s_active;
  const int size = activeDownloads.size();

  for(int i = 0; i < size; i++) {
    Download *download = activeDownloads[i];

    if(!download->isFinished())
      continue;

    // this need to be done before execCallback in case one of the callback
    // deletes the download object
    activeDownloads.erase(activeDownloads.begin() + i);

    download->execCallbacks();
  }

  if(Download::s_active.empty())
    plugin_register("-timer", (void*)timerTick);
}


Download::StartCode Download::start()
{
  if(m_threadHandle)
    return AlreadyRunning;

  s_active.push_back(this);
  plugin_register("timer", (void*)timerTick);

  m_threadHandle = CreateThread(NULL, 0, worker, (void *)this, 0, 0);
  return Started;
}

void Download::stop()
{
  WDL_MutexLock lock(&m_mutex);

  if(!m_threadHandle)
    return;

  m_stop = true;

  if(!isFinished())
    s_active.erase(std::remove(s_active.begin(), s_active.end(), this));

  WaitForSingleObject(m_threadHandle, INFINITE);
  CloseHandle(m_threadHandle);
  m_threadHandle = 0;

  m_stop = false;
};

DWORD WINAPI Download::worker(void *ptr) // static
{
  JNL::open_socketlib();

  Download *download = static_cast<Download *>(ptr);

  JNL_HTTPGet agent;
  agent.addheader("Accept: */*");
  agent.addheader("User-Agent: ReaPack (REAPER)");
  agent.connect(download->url());

  const time_t startTime = time(NULL);

  while(agent.run() == 0) {
    if(download->isCancelled()) {
      download->finish(-2, "");
      JNL::close_socketlib();
      return 1;
    }
    else if(time(NULL) - startTime >= DOWNLOAD_TIMEOUT) {
      download->finish(-3, "");
      JNL::close_socketlib();
      return 1;
    }

    Sleep(50);
  }

  const int status = agent.getreplycode();
  const int size = agent.bytes_available();

  char *buffer = new char[size];
  agent.get_bytes(buffer, size);

  download->finish(status, buffer);

  delete[] buffer;

  JNL::close_socketlib();

  return 0;
}

void Download::finish(const int status, const std::string &contents)
{
  WDL_MutexLock lock(&m_mutex);

  // called from the worker thread
  m_finished = true;
  m_status = status;
  m_contents = contents;
}

void Download::execCallbacks()
{
  // always called from the main thread

  for(DownloadCallback callback : m_callback)
    callback(status(), contents());
}

int Download::status()
{
  WDL_MutexLock lock(&m_mutex);

  return m_status;
}

const std::string &Download::contents()
{
  WDL_MutexLock lock(&m_mutex);

  return m_contents;
}

bool Download::isFinished()
{
  WDL_MutexLock lock(&m_mutex);

  return m_finished;
}

bool Download::isCancelled()
{
  WDL_MutexLock lock(&m_mutex);

  return m_stop;
}
