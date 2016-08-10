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

#include "download.hpp"

#include <curl/curl.h>

#include <reaper_plugin_functions.h>

using namespace std;

Download::Queue Download::s_finished;
WDL_Mutex Download::s_mutex;
size_t Download::s_running = 0;

static const int DOWNLOAD_TIMEOUT = 15;
static const int CONCURRENT_DOWNLOADS = 3;

void Download::Init()
{
  curl_global_init(CURL_GLOBAL_DEFAULT);
}

void Download::Cleanup()
{
  curl_global_cleanup();
}

Download::Download(const string &name, const string &url, const NetworkOpts &opts)
  : m_name(name), m_url(url), m_opts(opts), m_threadHandle(nullptr)
{
  reset();
}

Download::~Download()
{
  wait();
}

void Download::reset()
{
  m_state = Idle;
  m_aborted = false;
  m_contents.clear();
  m_progress = 0;
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

  m_state = Running;
  m_onStart();

  RegisterStart();
  m_threadHandle = CreateThread(nullptr, 0, Worker, (void *)this, 0, nullptr);
}

DWORD WINAPI Download::Worker(void *ptr)
{
  Download *download = static_cast<Download *>(ptr);

  string contents;

  char userAgent[64] = {};
#ifndef _WIN32
  snprintf(userAgent, sizeof(userAgent),
#else
  // _snwprintf doesn't append a null byte if len >= count
  // see https://msdn.microsoft.com/en-us/library/2ts7cx93.aspx
  _snprintf(userAgent, sizeof(userAgent) - 1,
#endif
    "ReaPack/%s (REAPER v%s)", "1.0", GetAppVersion());

  const NetworkOpts &opts = download->options();

  CURL *curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_URL, download->url().c_str());
  curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent);
  curl_easy_setopt(curl, CURLOPT_PROXY, opts.proxy.c_str());
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, opts.verifyPeer);

  curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1);
  curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, DOWNLOAD_TIMEOUT);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, DOWNLOAD_TIMEOUT);

  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, true);
  curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5);

  curl_easy_setopt(curl, CURLOPT_HEADER, true);
  curl_easy_setopt(curl, CURLOPT_NOPROGRESS, false);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &contents);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteData);
  curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, download);
  curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, UpdateProgress);

  const CURLcode res = curl_easy_perform(curl);

  if(download->isAborted()) {
    download->finish(Aborted, "aborted by user");
    curl_easy_cleanup(curl);
    return 1;
  }
  else if(res != CURLE_OK) {
    download->finish(Failure, curl_easy_strerror(res));
    curl_easy_cleanup(curl);
    return 1;
  }

  int status = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);

  // accept a status of 0 for the file:// protocol
  switch(status) {
  case 0: // for the file:// protocol
  case 200: // HTTP OK
  {
    // strip headers
    long headerSize = 0;
    curl_easy_getinfo(curl, CURLINFO_HEADER_SIZE, &headerSize);
    contents.erase(0, headerSize);

    download->finish(Success, contents);
    break;
  }
  default:
    // strip body, only keep error description
    contents.erase(contents.find("\r"));
    contents.erase(0, contents.find("\x20") + 1);

    download->finish(Failure, contents);
    break;
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

int Download::UpdateProgress(void *ptr, const double dltotal, const double dlnow,
    const double ultotal, const double ulnow)
{
  Download *dl = static_cast<Download *>(ptr);

  if(dl->isAborted())
    return 1;

  const double total = ultotal + dltotal;

  if(total > 0) {
    const short progress = (short)((ulnow + dlnow / total) * 100);
    dl->setProgress(min(progress, (short)100));
  }

  return 0;
}

void Download::RegisterStart()
{
  WDL_MutexLock lock(&s_mutex);

  if(!s_running)
    plugin_register("timer", (void*)TimerTick);

  s_running++;
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
    plugin_register("-timer", (void *)TimerTick);

  if(s_finished.empty())
    return nullptr;

  Download *dl = s_finished.front();
  s_finished.pop();

  s_running--;

  return dl;
}

void Download::setProgress(const short percent)
{
  WDL_MutexLock lock(&m_mutex);

  m_progress = percent;
}

void Download::finish(const State state, const string &contents)
{
  // called from the worker thread

  WDL_MutexLock lock(&m_mutex);

  m_state = state;
  m_contents = contents;

  MarkAsFinished(this);
}

void Download::finishInMainThread()
{
  if(m_threadHandle) {
    CloseHandle(m_threadHandle);
    m_threadHandle = nullptr;
  }

  m_onFinish();
  m_cleanupHandler();
}

auto Download::state() -> State
{
  WDL_MutexLock lock(&m_mutex);

  return m_state;
}

const string &Download::contents()
{
  WDL_MutexLock lock(&m_mutex);

  return m_contents;
}

bool Download::isAborted()
{
  WDL_MutexLock lock(&m_mutex);

  return m_aborted;
}

short Download::progress()
{
  WDL_MutexLock lock(&m_mutex);

  return m_progress;
}

void Download::abort()
{
  WDL_MutexLock lock(&m_mutex);

  m_aborted = true;
}

DownloadQueue::~DownloadQueue()
{
  // don't emit DownloadQueue::onAbort from the destructor
  // which is most likely to cause a crash
  m_onAbort.disconnect_all_slots();

  abort();
}

void DownloadQueue::push(Download *dl)
{
  m_onPush(dl);

  dl->onFinish([=] {
    m_running.erase(remove(m_running.begin(), m_running.end(), dl));

    start();
  });

  dl->setCleanupHandler([=] {
    delete dl;

    // call m_onDone() only after every onFinish slots ran
    if(idle())
      m_onDone();
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

  m_onAbort();
}

void DownloadQueue::clear()
{
  while(!m_queue.empty()) {
    Download *download = m_queue.front();
    delete download;

    m_queue.pop();
  }
}
