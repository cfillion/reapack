#include "download.hpp"

#include <curl/curl.h>

#include <reaper_plugin_functions.h>

using namespace std;

Download::Queue Download::s_finished;
WDL_Mutex Download::s_mutex;
size_t Download::s_running = 0;

static const int DOWNLOAD_TIMEOUT = 5;
static const int CONCURRENT_DOWNLOADS = 3;

Download::Download(const string &name, const string &url)
  : m_name(name), m_url(url), m_threadHandle(nullptr)
{
  reset();
}

Download::~Download()
{
  wait();
}

void Download::reset()
{
  m_aborted = false;
  m_finished = false;
  m_status = 0;
  m_contents.clear();
}

void Download::wait()
{
  if(m_threadHandle) {
    WaitForSingleObject(m_threadHandle, INFINITE);
    CloseHandle(m_threadHandle);
  }
}

void Download::TimerTick()
{
  Download *dl = Download::NextFinished();

  if(dl)
    dl->finishInMainThread();
}

void Download::start()
{
  if(m_threadHandle)
    return;

  reset();

  m_onStart();

  RegisterStart();
  m_threadHandle = CreateThread(nullptr, 0, Worker, (void *)this, 0, nullptr);
}

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
    contents.erase(0, contents.find("\x20") + 1);

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

void Download::RegisterStart()
{
  WDL_MutexLock lock(&s_mutex);

  s_running++;
  plugin_register("timer", (void*)TimerTick);
}

void Download::MarkAsFinished(Download *dl)
{
  WDL_MutexLock lock(&s_mutex);

  s_finished.push(dl);
}

Download *Download::NextFinished()
{
  WDL_MutexLock lock(&s_mutex);

  if(!s_running)
    plugin_register("-timer", (void*)TimerTick);

  if(s_finished.empty())
    return nullptr;

  Download *dl = s_finished.front();
  s_finished.pop();

  s_running--;

  return dl;
}

void Download::finish(const int status, const string &contents)
{
  // called from the worker thread

  WDL_MutexLock lock(&m_mutex);

  m_finished = true;
  m_status = status;
  m_contents = contents;

  MarkAsFinished(this);
}

void Download::finishInMainThread()
{
  WDL_MutexLock lock(&m_mutex);

  if(m_threadHandle) {
    CloseHandle(m_threadHandle);
    m_threadHandle = nullptr;
  }

  m_onFinish();
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
  abort();
}

void DownloadQueue::push(Download *dl)
{
  m_onPush(dl);

  dl->onFinish([=]() {
    m_running.erase(remove(m_running.begin(), m_running.end(), dl));
    delete dl;

    start();
  });

  m_queue.push(dl);

  start();
}

void DownloadQueue::start()
{
  while(m_running.size() < CONCURRENT_DOWNLOADS && !m_queue.empty()) {
    Download *dl = m_queue.front();
    m_queue.pop();

    m_running.push_back(dl);
    dl->start();
  }
}

void DownloadQueue::abort()
{
  for(Download *dl : m_running)
    dl->abort();

  clear();
}

void DownloadQueue::clear()
{
  while(!m_queue.empty()) {
    Download *download = m_queue.front();
    delete download;

    m_queue.pop();
  }
}
