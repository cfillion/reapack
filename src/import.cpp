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

#include "import.hpp"

#include "config.hpp"
#include "download.hpp"
#include "errors.hpp"
#include "filesystem.hpp"
#include "index.hpp"
#include "reapack.hpp"
#include "remote.hpp"
#include "resource.hpp"
#include "transaction.hpp"

#include <boost/algorithm/string/trim.hpp>

using namespace std;

static const Char *TITLE = L("ReaPack: Import repositories");
static const String DISCOVER_URL = "https://reapack.com/repos";

Import::Import()
  : Dialog(IDD_IMPORT_DIALOG), m_pool(nullptr), m_state(OK)
{
}

void Import::onInit()
{
  Dialog::onInit();

  SetWindowText(handle(), TITLE);

  m_url = getControl(IDC_URL);
  m_progress = getControl(IDC_PROGRESS);
  m_discover = getControl(IDC_DISCOVER);

#ifdef PBM_SETMARQUEE
  SendMessage(m_progress, PBM_SETMARQUEE, 1, 0);
#endif
}

void Import::onCommand(const int id, int)
{
  switch(id) {
  case IDOK:
    fetch();
    break;
  case IDC_DISCOVER:
    openURL(DISCOVER_URL);
    break;
  case IDCANCEL:
    if(m_pool) {
      disable(getControl(IDOK));
      disable(getControl(IDCANCEL));
      m_pool->abort();
      m_state = Close;
    }
    else
      close();
    break;
  }
}

void Import::onTimer(int)
{
#ifndef PBM_SETMARQUEE
  m_fakePos = (m_fakePos + 1) % 100;
  SendMessage(m_progress, PBM_SETPOS, m_fakePos, 0);
#endif
}

ThreadPool *Import::setupPool()
{
  if(!m_pool) {
    m_state = OK;
    m_pool = new ThreadPool;

    m_pool->onAbort([=] { if(!m_state) m_state = Aborted; });
    m_pool->onDone([=] {
      setWaiting(false);

      if(!m_state)
        processQueue();

      m_queue.clear();

      delete m_pool;
      m_pool = nullptr;

      if(m_state == Close)
        close();
      else
        SetFocus(m_url);
    });
  }

  return m_pool;
}

void Import::fetch()
{
  if(m_pool) // ignore repeated presses on OK
    return;

  const auto &opts = g_reapack->config()->network;

  size_t index = 0;
  StringStream stream(getText(m_url));
  String url;
  while(getline(stream, url)) {
    boost::algorithm::trim(url);

    if(url.empty())
      continue;

    MemoryDownload *dl = new MemoryDownload(url, opts);
    ++index;

    dl->onFinish([=] {
      switch(dl->state()) {
      case ThreadTask::Success:
        // copy for later use, as `dl` won't be around after this callback
        if(!read(dl, index))
          m_pool->abort();
        break;
      case ThreadTask::Failure: {
        Char msg[1024];
        snprintf(msg, lengthof(msg), L("Download failed: %s\r\nn%s"),
          dl->error().message.c_str(), url.c_str());
        MessageBox(handle(), msg, TITLE, MB_OK);
        m_pool->abort();
        break;
      }
      default:
        break;
      }
    });

    setupPool()->push(dl);
  }

  if(m_pool)
    setWaiting(true);
  else
    close();
}

bool Import::read(MemoryDownload *dl, const size_t idx)
{
  Char msg[1024];

  try {
    IndexPtr index = Index::load({}, dl->contents().c_str());
    Remote remote = g_reapack->remote(index->name());
    const bool exists = !remote.isNull();

    if(exists && remote.url() != dl->url()) {
      if(remote.isProtected()) {
        snprintf(msg, lengthof(msg),
          L("The repository %s is protected and cannot be overwritten."),
          index->name().c_str());
        MessageBox(handle(), msg, TITLE, MB_OK);
        return false;
      }
      else {
        snprintf(msg, lengthof(msg),
          L("%s is already configured with a different URL.\r\n")
          L("Do you want to overwrite it?"),
          index->name().c_str());

        const auto answer = MessageBox(handle(), msg, TITLE, MB_YESNO);

        if(answer != IDYES)
          return true;
      }
    }
    else if(exists && remote.isEnabled()) // url is also the same
      return true; // nothing to do

    remote.setName(index->name());
    remote.setUrl(dl->url());
    m_queue.push_back({idx, remote, dl->contents()});

    return true;
  }
  catch(const reapack_error &e) {
    snprintf(msg, lengthof(msg),
      L("The received file is invalid: %s\r\n%s"),
      e.what().c_str(), dl->url().c_str());
    MessageBox(handle(), msg, TITLE, MB_OK);
    return false;
  }
}

void Import::processQueue()
{
  bool ok = true, commit = !m_queue.empty();

  sort(m_queue.begin(), m_queue.end());

  while(!m_queue.empty()) {
    if(!import(m_queue.front()))
      ok = false;

    m_queue.pop_front();
  }

  if(ok)
    m_state = Close;

  if(commit)
    g_reapack->commitConfig();
}

bool Import::import(const ImportData &data)
{
  if(!g_reapack->addSetRemote(data.remote))
    return false;

  FS::write(Index::pathFor(data.remote.name()), data.contents);

  return true;
}

void Import::setWaiting(const bool wait)
{
  setVisible(wait, m_progress);
  setVisible(!wait, m_discover);
  setEnabled(!wait, m_url);

#ifndef PBM_SETMARQUEE
  const int timerId = 1;

  if(wait)
    startTimer(42, timerId);
  else
    stopTimer(timerId);

  m_fakePos = 0;
  SendMessage(m_progress, PBM_SETPOS, 0, 0);
#endif
}
