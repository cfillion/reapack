/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015  Christian Fillion
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

#include "progress.hpp"

#include "download.hpp"
#include "resource.hpp"
#include "transaction.hpp"

static const auto_char *TITLE = AUTO_STR("ReaPack: Download in progress");

using namespace std;

Progress::Progress()
  : Dialog(IDD_PROGRESS_DIALOG),
    m_transaction(nullptr), m_label(nullptr), m_progress(nullptr),
    m_done(0), m_total(0)
{
}

void Progress::setTransaction(Transaction *t)
{
  m_transaction = t;

  m_done = 0;
  m_total = 0;
  m_currentName.clear();

  if(!m_transaction)
    return;

  SetWindowText(m_label, AUTO_STR("Initializing..."));

  m_transaction->downloadQueue()->onPush(
    bind(&Progress::addDownload, this, placeholders::_1));
}

void Progress::onInit()
{
  m_label = getItem(IDC_LABEL);
  m_progress = GetDlgItem(handle(), IDC_PROGRESS);
}

void Progress::onCommand(WPARAM wParam, LPARAM)
{
  switch(LOWORD(wParam)) {
  case IDCANCEL:
    if(m_transaction)
      m_transaction->cancel();

    // don't wait until the current downloads are finished
    // before getting out of the user way
    hide();
    break;
  }
}

void Progress::addDownload(Download *dl)
{
  m_total++;
  updateProgress();

  dl->onStart([=] {
    m_currentName = make_autostring(dl->name());
    updateProgress();
  });

  dl->onFinish([=] {
    m_done++;
    updateProgress();
  });
}

void Progress::updateProgress()
{
  const auto_string text = AUTO_STR("Downloading ") +
    to_autostring(min(m_done + 1, m_total)) + AUTO_STR(" of ") +
    to_autostring(m_total) + AUTO_STR(": ") + m_currentName;

  SetWindowText(m_label, text.c_str());

  const double pos = (double)m_done / m_total;
  const int percent = (int)(pos * 100);

  const auto_string title = auto_string(TITLE) +
    AUTO_STR(" (") + to_autostring(percent) + AUTO_STR("%)");

  SendMessage(m_progress, PBM_SETPOS, percent, 0);
  SetWindowText(handle(), title.c_str());
}
