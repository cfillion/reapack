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
  m_running = false;
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

  m_running = true;
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

  m_running = false;
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
  m_onDestroy();
}

bool Download::isRunning()
{
  WDL_MutexLock lock(&m_mutex);

  return m_running;
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

  dl->onFinish([=] {
    m_running.erase(remove(m_running.begin(), m_running.end(), dl));

    start();
  });

  dl->onDestroy([=] {
     delete dl;

    if(idle())
      m_onDone(nullptr);
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
