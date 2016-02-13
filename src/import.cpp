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
#include "reapack.hpp"
#include "remote.hpp"
#include "resource.hpp"

#include <cerrno>
#include <sstream>

#include <reaper_plugin_functions.h>

#ifndef _WIN32 // for SWELL
#define SetWindowTextA SetWindowText
#endif

using namespace std;

const char *Import::TITLE = "ReaPack: Import a repository";
const char *SUBTITLE = "Select a .ReaPackRemote file";

Import::Import(ReaPack *reapack)
  : Dialog(IDD_IMPORT_DIALOG), m_reapack(reapack), m_download(nullptr)
{
}

void Import::onInit()
{
  SetWindowTextA(handle(), TITLE);
  SetWindowTextA(getControl(IDC_GROUPBOX), SUBTITLE);

  m_url = getControl(IDC_URL);
  m_file = getControl(IDC_FILE);
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
  case IDC_BROWSE:
    browseFile();
    break;
  case IDOK:
    import();
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

void Import::browseFile()
{
  string path(4096, 0);
  if(!GetUserFileNameForRead(&path[0], SUBTITLE, "ReaPackRemote")) {
    SetFocus(handle());
    return;
  }

  SetWindowText(m_file, make_autostring(path).c_str());
  import(true);
}

void Import::import(const bool fileOnly)
{
  auto_string input(4096, 0);

  if(!fileOnly) {
    GetWindowText(m_url, &input[0], (int)input.size());

    if(input[0] != 0) {
      download(from_autostring(input));
      return;
    }
  }

  GetWindowText(m_file, &input[0], (int)input.size());

  if(input[0] == 0) {
    close();
    return;
  }

  Remote::ReadCode code;
  if(!import(Remote::fromFile(from_autostring(input), &code), code))
    SetFocus(m_file);
}

bool Import::import(const Remote &remote, const Remote::ReadCode code)
{
  switch(code) {
  case Remote::ReadFailure:
    ShowMessageBox(strerror(errno), TITLE, 0);
    return false;
  case Remote::InvalidName:
    ShowMessageBox("Invalid .ReaPackRemote file! (invalid name)", TITLE, 0);
    return false;
  case Remote::InvalidUrl:
    ShowMessageBox("Invalid .ReaPackRemote file! (invalid url)", TITLE, 0);
    return false;
  default:
    break;
  };

  m_reapack->import(remote);
  close();

  return true;
}

void Import::download(const string &url)
{
  if(m_download)
   return;

  setWaiting(true);

  Download *dl = new Download({}, url);
  dl->onFinish([=] {
    const Download::State state = dl->state();
    if(state == Download::Aborted) {
      // we are deleted here, so there is nothing much we can do without crashing
      return; 
    }

    setWaiting(false);

    if(state != Download::Success) {
      ShowMessageBox(dl->contents().c_str(), "Download Failed", MB_OK);
      SetFocus(m_url);
      return;
    }

    istringstream stream(dl->contents());

    Remote::ReadCode code;
    if(!import(Remote::fromFile(stream, &code), code))
      SetFocus(m_url);
  });

  dl->onDestroy([=] {
    // if we are still alive
    if(dl->state() != Download::Aborted)
      m_download = nullptr;

    delete dl;
  });

  dl->start();

  m_download = dl;
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
