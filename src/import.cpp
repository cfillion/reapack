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

#include "config.hpp"
#include "download.hpp"
#include "errors.hpp"
#include "filesystem.hpp"
#include "index.hpp"
#include "reapack.hpp"
#include "remote.hpp"
#include "resource.hpp"
#include "transaction.hpp"

using namespace std;

static const auto_char *TITLE = AUTO_STR("ReaPack: Import a repository");

Import::Import(ReaPack *reapack)
  : Dialog(IDD_IMPORT_DIALOG), m_reapack(reapack), m_download(nullptr)
{
}

void Import::onInit()
{
  SetWindowText(handle(), TITLE);

  m_url = getControl(IDC_URL);
  m_progress = getControl(IDC_PROGRESS);
  m_ok = getControl(IDOK);

  hide(m_progress);

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

  // remove extra nulls from the string
  url.resize(url.find(AUTO_STR('\0')));

  if(url.empty()) {
    close();
    return;
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
      const string msg = "Download failed: " + dl->contents();
      MessageBox(handle(), make_autostring(msg).c_str(), TITLE, MB_OK);
      SetFocus(m_url);
      return;
    }

    read();
  });

  dl->setCleanupHandler([=] {
    // if we are still alive
    if(dl->state() != Download::Aborted)
      m_download = nullptr;

    delete dl;
  });

  dl->start();
}

void Import::read()
{
  assert(m_download);

  try {
    IndexPtr index = Index::load({}, m_download->contents().c_str());
    close(import({index->name(), m_download->url()}));
  }
  catch(const reapack_error &e) {
    const string msg = "The received file is invalid: " + string(e.what());
    MessageBox(handle(), make_autostring(msg).c_str(), TITLE, MB_OK);
    SetFocus(m_url);
  }
}

bool Import::import(const Remote &remote)
{
  if(const Remote &existing = m_reapack->remote(remote.name())) {
    if(existing.isProtected()) {
      MessageBox(handle(),
        AUTO_STR("This repository is protected and cannot be overwritten."),
        TITLE, MB_OK);

      return false;
    }
    else if(existing.url() != remote.url()) {
      auto_char msg[1024] = {};
      auto_snprintf(msg, auto_size(msg),
        AUTO_STR("%s is already configured with a different URL.\r\n")
        AUTO_STR("Do you want to overwrite it?"),
        make_autostring(remote.name()).c_str());

      const auto answer = MessageBox(handle(), msg, TITLE, MB_YESNO);

      if(answer != IDYES)
        return false;
    }
    else if(existing.isEnabled()) {
      auto_char msg[1024] = {};
      auto_snprintf(msg, auto_size(msg),
        AUTO_STR("%s is already configured.\r\nNothing to do!"),
        make_autostring(remote.name()).c_str());
      MessageBox(handle(), msg, TITLE, MB_OK);

      return false;
    }
    else {
      Transaction *tx = m_reapack->setupTransaction();

      if(!tx)
        return true;

      m_reapack->enable(existing);
      tx->runTasks();

      m_reapack->config()->write();

      auto_char msg[1024] = {};
      auto_snprintf(msg, auto_size(msg), AUTO_STR("%s has been enabled."),
        make_autostring(remote.name()).c_str());
      MessageBox(handle(), msg, TITLE, MB_OK);

      return true;
    }
  }

  Config *config = m_reapack->config();
  config->remotes()->add(remote);
  config->write();

  FS::write(Index::pathFor(remote.name()), m_download->contents());

  auto_char msg[1024] = {};
  auto_snprintf(msg, auto_size(msg),
    AUTO_STR("%s has been successfully imported into your repository list."),
    make_autostring(remote.name()).c_str());
  MessageBox(handle(), msg, TITLE, MB_OK);

  m_reapack->refreshManager();
  m_reapack->refreshBrowser();

  return true;
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
