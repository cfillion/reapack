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

#ifndef REAPACK_PROGRESS_HPP
#define REAPACK_PROGRESS_HPP

#include "dialog.hpp"

#include "encoding.hpp"

class Download;
class DownloadQueue;

class Progress : public Dialog {
public:
  Progress(DownloadQueue *);

protected:
  void onInit() override;
  void onCommand(short, short) override;

private:
  void addDownload(Download *);
  void updateProgress();

  DownloadQueue *m_queue;
  auto_string m_currentName;

  HWND m_label;
  HWND m_progress;

  int m_done;
  int m_total;
};

#endif
