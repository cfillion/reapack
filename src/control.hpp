/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2025  Christian Fillion
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

#ifndef REAPACK_CONTROL_HPP
#define REAPACK_CONTROL_HPP

#ifdef _WIN32
#  include <windows.h>
#else
#  include <swell/swell.h>
#endif

class Dialog;

class Control {
public:
  Control(HWND handle) : m_handle(handle) {}
  virtual ~Control() = default;

  HWND handle() const { return m_handle; }

protected:
  friend Dialog;

  virtual void onNotify(LPNMHDR, LPARAM) {}
  virtual bool onContextMenu(HWND, int, int) { return false; }

private:
  HWND m_handle;
};

class InhibitControl {
public:
  InhibitControl(Control *ctrl) : InhibitControl(ctrl->handle()) {}
  InhibitControl(HWND handle) : m_handle(handle) { setRedraw(true); }
  ~InhibitControl() { setRedraw(false); }

private:
  void setRedraw(const bool inhibit);

  HWND m_handle;
};

#endif
