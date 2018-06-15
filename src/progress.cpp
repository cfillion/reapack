/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2018  Christian Fillion
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

#include "thread.hpp"
#include "resource.hpp"
#include "win32.hpp"

#include <sstream>

using namespace std;

Progress::Progress(ThreadPool *pool)
  : Dialog(IDD_PROGRESS_DIALOG),
    m_pool(pool), m_label(nullptr), m_progress(nullptr),
    m_done(0), m_total(0)
{
  m_pool->onPush(bind(&Progress::addTask, this, _1));
}

void Progress::onInit()
{
  Dialog::onInit();

  m_label = getControl(IDC_LABEL);
  m_progress = getControl(IDC_PROGRESS);

  Win32::setWindowText(m_label, "Initializing...");
}

void Progress::onCommand(const int id, int)
{
  switch(id) {
  case IDCANCEL:
    m_pool->abort();

    // don't wait until the current downloads are finished
    // before getting out of the user way
    hide();
    break;
  }
}

void Progress::onTimer(const int id)
{
#ifdef _WIN32
  if(!IsWindowEnabled(handle()))
    return;
#endif

  show();
  stopTimer(id);
}

void Progress::addTask(ThreadTask *task)
{
  m_total++;
  updateProgress();

  if(!isVisible())
    startTimer(100);

  task->onStart([=] {
    m_current = task->summary();
    updateProgress();
  });

  task->onFinish([=] {
    m_done++;
    updateProgress();
  });
}

void Progress::updateProgress()
{
  ostringstream position;
  String::imbueStream(position);
  position << min(m_done + 1, m_total) << " of " << m_total;

  char label[1024];
  snprintf(label, sizeof(label), m_current.c_str(), position.str().c_str());

  Win32::setWindowText(m_label, label);

  const double pos = (double)(min(m_done+1, m_total)) / max(2, m_total);
  const int percent = (int)(pos * 100);

  char title[255];
  snprintf(title, sizeof(title),
    "ReaPack: Operation in progress (%d%%)", percent);

  SendMessage(m_progress, PBM_SETPOS, percent, 0);
  Win32::setWindowText(handle(), title);
}
