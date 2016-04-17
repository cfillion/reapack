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

#ifndef REAPACK_IMPORT_HPP
#define REAPACK_IMPORT_HPP

#include "dialog.hpp"

#include "encoding.hpp"

#include <string>

class Download;
class ReaPack;

class Import : public Dialog
{
public:
  static const auto_char *TITLE;

  Import(ReaPack *);

protected:
  void onInit() override;
  void onCommand(int, int) override;
  void onTimer(int) override;

private:
  void fetch();
  bool import();
  void setWaiting(bool);

  ReaPack *m_reapack;
  Download *m_download;
  short m_fakePos;

  HWND m_url;
  HWND m_progress;
  HWND m_ok;
};

#endif
