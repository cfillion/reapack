#include "download.hpp"

#include <curl/curl.h>

#include <reaper_plugin_functions.h>

using namespace std;

vector<Download *> Download::s_active;

static const int DOWNLOAD_TIMEOUT = 5;

Download::Download(const std::string &name, const std::string &url)
  : m_threadHandle(0), m_name(name), m_url(url)
{
  reset();
}

Download::~Download()
{
  if(!isFinished())
    s_active.erase(remove(s_active.begin(), s_active.end(), this));

  // call stop after removing from the active list to prevent
  // bad access from timeTick -> execCallbacks
  stop();

  // free the content buffer
  reset();
}

void Download::reset()
{
  m_aborted = false;
  m_finished = false;
  m_status = 0;
  m_contents.clear();
}

void Download::addCallback(const DownloadCallback &callback)
{
  m_callback.push_back(callback);
}

void Download::TimerTick()
{
  vector<Download *> &activeDownloads = Download::s_active;
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
    plugin_register("-timer", (void*)TimerTick);
}

void Download::start()
{
  if(m_threadHandle)
    return;

  reset();

  s_active.push_back(this);
  plugin_register("timer", (void*)TimerTick);

  m_threadHandle = CreateThread(NULL, 0, Worker, (void *)this, 0, 0);
}

void Download::stop()
{
  if(!m_threadHandle)
    return;

  abort();

  WaitForSingleObject(m_threadHandle, INFINITE);
  CloseHandle(m_threadHandle);
  m_threadHandle = 0;

  // do not call reset() here, m_finished must stay to true
  // otherwise the callback will not be called
};

DWORD WINAPI Download::Worker(void *ptr)
{
  Download *download = static_cast<Download *>(ptr);

  string contents;

  CURL *curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_URL, download->url().c_str());
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "ReaPack/1.0 (REAPER)");
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, DOWNLOAD_TIMEOUT);

  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, true);
  curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 1);

  curl_easy_setopt(curl, CURLOPT_HEADER, true);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &contents);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteData);

  const CURLcode res = curl_easy_perform(curl);

  if(download->isAborted()) {
    download->finish(-2, "aborted");
    curl_easy_cleanup(curl);
    return 1;
  }
  else if(res != CURLE_OK) {
    download->finish(0, curl_easy_strerror(res));
    curl_easy_cleanup(curl);
    return 1;
  }

  int status = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);

  if(status == 200) {
    // strip headers
    long headerSize = 0;
    curl_easy_getinfo(curl, CURLINFO_HEADER_SIZE, &headerSize);
    contents.erase(0, headerSize);

    download->finish(status, contents);
  }
  else {
    // strip body
    contents.erase(contents.find("\n"));

    download->finish(status, contents);
  }

  curl_easy_cleanup(curl);

  return 0;
}

size_t Download::WriteData(char *ptr, size_t rawsize, size_t nmemb, void *data)
{
  const size_t size = rawsize * nmemb;

  string *str = static_cast<string *>(data);
  str->append(ptr, size);

  return size;
}


void Download::finish(const int status, const string &contents)
{
  WDL_MutexLock lock(&m_mutex);

  // called from the worker thread
  m_finished = true;
  m_status = status;
  m_contents = contents;
}

void Download::execCallbacks()
{
  WDL_MutexLock lock(&m_mutex);

  // always called from the main thread from timerTick when m_finished is true

  if(m_threadHandle && m_finished) {
    CloseHandle(m_threadHandle);
    m_threadHandle = 0;
  }

  for(DownloadCallback callback : m_callback)
    callback(this);
}

bool Download::isFinished()
{
  WDL_MutexLock lock(&m_mutex);

  return m_finished;
}

bool Download::isAborted()
{
  WDL_MutexLock lock(&m_mutex);

  return m_aborted;
}

void Download::abort()
{
  WDL_MutexLock lock(&m_mutex);

  m_aborted = true;
}

DownloadQueue::~DownloadQueue()
{
  while(!m_queue.empty()) {
    Download *download = m_queue.front();
    download->stop();
    // delete called in the stop callback

    m_queue.pop();
  }
}

void DownloadQueue::push(const string &name,
  const string &url, DownloadCallback cb)
{
  Download *download = new Download(name, url);
  download->addCallback(cb);

  download->addCallback([=](Download *dl) {
    m_queue.pop();
    delete dl;

    if(!m_queue.empty())
      m_queue.front()->start();
  });

  m_queue.push(download);

  if(m_queue.size() == 1)
    download->start();
}
