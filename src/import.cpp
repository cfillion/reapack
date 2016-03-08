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

#include "import.hpp"

#include "download.hpp"
#include "encoding.hpp"
#include "errors.hpp"
#include "index.hpp"
#include "reapack.hpp"
#include "remote.hpp"
#include "resource.hpp"

#include <reaper_plugin_functions.h>

#ifndef _WIN32 // for SWELL
#define SetWindowTextA SetWindowText
#endif

using namespace std;

const char *Import::TITLE = "ReaPack: Import a repository";

Import::Import(ReaPack *reapack)
  : Dialog(IDD_IMPORT_DIALOG), m_reapack(reapack), m_download(nullptr)
{
}

void Import::onInit()
{
  SetWindowTextA(handle(), TITLE);

  m_url = getControl(IDC_URL);
  m_progress = getControl(IDC_PROGRESS);
  m_ok = getControl(IDOK);

  hide(m_progress);

#ifdef PBM_SETMARQUEE
  SendMessage(m_progress, PBM_SETMARQUEE, 1, 0);
#endif
}

void Import::onCommand(const int id)
{
  switch(id) {
  case IDOK:
    fetch();
    break;
  case IDCANCEL:
    if(m_download)
      m_download->abort();

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

void Import::fetch()
{
  if(m_download)
   return;

  auto_string url(4096, 0);
  GetWindowText(m_url, &url[0], (int)url.size());

  const size_t end = url.find(AUTO_STR('\0'));

  if(end == 0) { // url is empty
    close();
    return;
  }
  else {
    // remove extra nulls from the string
    url.resize(end);
  }

  setWaiting(true);

  Download *dl = m_download = new Download({}, from_autostring(url));

  dl->onFinish([=] {
    const Download::State state = dl->state();
    if(state == Download::Aborted) {
      // at this point `this` is deleted, so there is nothing else
      // we can do without crashing
      return; 
    }

    setWaiting(false);

    if(state != Download::Success) {
      ShowMessageBox(dl->contents().c_str(), "Download Failed", MB_OK);
      SetFocus(m_url);
      return;
    }

    if(!import())
      SetFocus(m_url);
  });

  dl->setCleanupHandler([=] {
    // if we are still alive
    if(dl->state() != Download::Aborted)
      m_download = nullptr;

    delete dl;
  });

  dl->start();
}

bool Import::import()
{
  assert(m_download);

  try {
    IndexPtr index = Index::load({}, m_download->contents().c_str());
    m_reapack->import({index->name(), m_download->url()});

    close();

    return true;
  }
  catch(const reapack_error &e) {
    const string msg = "The received file is invalid: " + string(e.what());
    ShowMessageBox(msg.c_str(), TITLE, MB_OK);
    return false;
  }
}

void Import::setWaiting(const bool wait)
{
  setVisible(wait, m_progress);
  setEnabled(!wait, m_url);

#ifndef PBM_SETMARQUEE
  if(wait)
    startTimer(42, 1);
  else
    stopTimer(1);

  m_fakePos = 0;
  SendMessage(m_progress, PBM_SETPOS, 0, 0);
#endif
}
