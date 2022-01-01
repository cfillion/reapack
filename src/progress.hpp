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

#ifndef REAPACK_PROGRESS_HPP
#define REAPACK_PROGRESS_HPP

#include "dialog.hpp"

class ThreadPool;
class ThreadTask;

class Progress : public Dialog {
public:
  Progress(ThreadPool *);

protected:
  void onInit() override;
  void onCommand(int, int) override;
  void onTimer(int) override;

private:
  void addTask(ThreadTask *);
  void updateProgress();

  ThreadPool *m_pool;
  std::string m_current;

  HWND m_label;
  HWND m_progress;

  int m_done;
  int m_total;
};

#endif
