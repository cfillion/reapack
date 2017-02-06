/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2017  Christian Fillion
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

#include "reapack.hpp"

#include <boost/format.hpp>

#include <reaper_plugin_functions.h>

using boost::format;
using namespace std;

static const int DOWNLOAD_TIMEOUT = 15;

static CURLSH *g_curlShare = nullptr;
static WDL_Mutex g_curlMutex;

DownloadNotifier *DownloadNotifier::s_instance = nullptr;

static void LockCurlMutex(CURL *, curl_lock_data, curl_lock_access, void *)
{
  g_curlMutex.Enter();
}

static void UnlockCurlMutex(CURL *, curl_lock_data, curl_lock_access, void *)
{
  g_curlMutex.Leave();
}

void Download::Init()
{
  curl_global_init(CURL_GLOBAL_DEFAULT);

  g_curlShare = curl_share_init();
  assert(g_curlShare);

  curl_share_setopt(g_curlShare, CURLSHOPT_LOCKFUNC, LockCurlMutex);
  curl_share_setopt(g_curlShare, CURLSHOPT_UNLOCKFUNC, UnlockCurlMutex);

  curl_share_setopt(g_curlShare, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);
  curl_share_setopt(g_curlShare, CURLSHOPT_SHARE, CURL_LOCK_DATA_SSL_SESSION);
}

void Download::Cleanup()
{
  curl_share_cleanup(g_curlShare);
  curl_global_cleanup();
}

size_t Download::WriteData(char *ptr, size_t rawsize, size_t nmemb, void *data)
{
  const size_t size = rawsize * nmemb;

  string *str = static_cast<string *>(data);
  str->append(ptr, size);

  return size;
}

int Download::UpdateProgress(void *ptr, const double, const double,
    const double, const double)
{
  Download *dl = static_cast<Download *>(ptr);

  if(dl->isAborted())
    return 1;

  return 0;
}

Download::Download(const string &name, const string &url,
  const NetworkOpts &opts, const int flags)
  : m_name(name), m_url(url), m_opts(opts), m_flags(flags)
{
  reset();

  DownloadNotifier::get()->start();
}

Download::~Download()
{
  DownloadNotifier::get()->stop();
}

void Download::reset()
{
  m_state = Idle;
  m_aborted = false;
  m_contents.clear();
}

void Download::finish(const State state, const string &contents)
{
  m_contents = contents;

  DownloadNotifier::get()->notify({this, state});
}

void Download::setState(const State state)
{
  m_state = state;

  switch(state) {
  case Idle:
    break;
  case Running:
    m_onStart();
    break;
  case Success:
  case Failure:
  case Aborted:
    m_onFinish();
    m_cleanupHandler();
    break;
  }
}

void Download::start()
{
  DownloadThread *thread = new DownloadThread;
  thread->push(this);
  onFinish([thread] { delete thread; });
}

void Download::exec(CURL *curl)
{
  DownloadNotifier::get()->notify({this, Running});

  string contents;

  curl_easy_setopt(curl, CURLOPT_URL, m_url.c_str());
  curl_easy_setopt(curl, CURLOPT_PROXY, m_opts.proxy.c_str());
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, m_opts.verifyPeer);

  curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, UpdateProgress);
  curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, this);

  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteData);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &contents);

  curl_slist *headers = nullptr;
  if(has(Download::NoCacheFlag))
    headers = curl_slist_append(headers, "Cache-Control: no-cache");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  char errbuf[CURL_ERROR_SIZE];
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);

  const CURLcode res = curl_easy_perform(curl);

  if(isAborted())
    finish(Aborted, "aborted by user");
  else if(res != CURLE_OK) {
    const auto err = format("%s (%d): %s") % curl_easy_strerror(res) % res % errbuf;
    finish(Failure, err.str());
  }
  else
    finish(Success, contents);

  curl_slist_free_all(headers);
}

DownloadThread::DownloadThread() : m_exit(false)
{
  m_wake = CreateEvent(nullptr, true, false, AUTO_STR("WakeEvent"));
  m_thread = CreateThread(nullptr, 0, thread, (void *)this, 0, nullptr);
}

DownloadThread::~DownloadThread()
{
  // remove all pending downloads then wake the thread to make it exit
  m_exit = true;
  SetEvent(m_wake);

  WaitForSingleObject(m_thread, INFINITE);

  CloseHandle(m_wake);
  CloseHandle(m_thread);
}

DWORD WINAPI DownloadThread::thread(void *ptr)
{
  DownloadThread *thread = static_cast<DownloadThread *>(ptr);
  CURL *curl = curl_easy_init();

  const auto userAgent = format("ReaPack/%s REAPER/%s")
    % ReaPack::VERSION % GetAppVersion();

  curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent.str().c_str());
  curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1);
  curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, DOWNLOAD_TIMEOUT);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, DOWNLOAD_TIMEOUT);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, true);
  curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5);
  curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
  curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);
  curl_easy_setopt(curl, CURLOPT_SHARE, g_curlShare);
  curl_easy_setopt(curl, CURLOPT_NOPROGRESS, false);

  while(!thread->m_exit) {
    while(Download *dl = thread->nextDownload())
      dl->exec(curl);

    ResetEvent(thread->m_wake);
    WaitForSingleObject(thread->m_wake, INFINITE);
  }

  curl_easy_cleanup(curl);

  return 0;
}

Download *DownloadThread::nextDownload()
{
  WDL_MutexLock lock(&m_mutex);

  if(m_queue.empty())
    return nullptr;

  Download *dl = m_queue.front();
  m_queue.pop();
  return dl;
}

void DownloadThread::push(Download *dl)
{
  WDL_MutexLock lock(&m_mutex);
  m_queue.push(dl);
  SetEvent(m_wake);
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
  m_running.insert(dl);

  dl->setCleanupHandler([=] {
    delete dl;

    m_running.erase(dl);

    // call m_onDone() only after every onFinish slots ran
    if(m_running.empty())
      m_onDone();
  });

  auto &thread = m_pool[m_running.size() % m_pool.size()];
  if(!thread)
    thread = make_unique<DownloadThread>();

  thread->push(dl);
}

void DownloadQueue::abort()
{
  for(Download *dl : m_running)
    dl->abort();

  m_onAbort();
}

DownloadNotifier *DownloadNotifier::get()
{
  if(!s_instance)
    s_instance = new DownloadNotifier;

  return s_instance;
}

void DownloadNotifier::start()
{
  if(!m_active++)
    plugin_register("timer", (void *)tick);
}

void DownloadNotifier::stop()
{
  --m_active;
}

void DownloadNotifier::notify(const Notification &notif)
{
  WDL_MutexLock lock(&m_mutex);

  m_queue.push(notif);
}

void DownloadNotifier::tick()
{
  DownloadNotifier *instance = DownloadNotifier::get();
  instance->processQueue();

  // doing this in stop() would cause a use after free of m_mutex in processQueue
  if(!instance->m_active) {
    plugin_register("-timer", (void *)tick);

    delete s_instance;
    s_instance = nullptr;
  }
}

void DownloadNotifier::processQueue()
{
  WDL_MutexLock lock(&m_mutex);

  while(!m_queue.empty()) {
    const Notification &notif = m_queue.front();
    notif.first->setState(notif.second);
    m_queue.pop();
  }
}
