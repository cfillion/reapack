/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2022  Christian Fillion
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

Progress::Progress(ThreadPool *pool)
  : Dialog(IDD_PROGRESS_DIALOG),
    m_pool(pool), m_label(nullptr), m_progress(nullptr),
    m_done(0), m_total(0)
{
  m_pool->onPush >> std::bind(&Progress::addTask, this, std::placeholders::_1);
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

  task->onStartAsync >> [=] {
    m_current = task->summary();
    updateProgress();
  };

  task->onFinishAsync >> [=] {
    m_done++;
    updateProgress();
  };
}

void Progress::updateProgress()
{
  Win32::setWindowText(m_label, String::format(m_current.c_str(),
    String::format("%s of %s",
      String::number(std::min(m_done + 1, m_total)).c_str(),
      String::number(m_total).c_str()
    ).c_str()
  ).c_str());

  const double pos = static_cast<double>(
    std::min(m_done + 1, m_total)) / std::max(2, m_total);
  const int percent = static_cast<int>(pos * 100);

  SendMessage(m_progress, PBM_SETPOS, percent, 0);
  Win32::setWindowText(handle(), String::format(
    "ReaPack: Operation in progress (%d%%)", percent
  ).c_str());
}
