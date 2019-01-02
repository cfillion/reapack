/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2019  Christian Fillion
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

#include "control.hpp"

using namespace std;

map<HWND, InhibitControl *> InhibitControl::s_lock;

void InhibitControl::inhibitRedraw(const bool inhibit)
{
#ifdef _WIN32
  if(s_lock.count(m_handle)) {
    if(inhibit || s_lock[m_handle] != this)
      return;
  }
  else if(!inhibit)
    return;

  SendMessage(m_handle, WM_SETREDRAW, !inhibit, 0);

  if(inhibit)
    s_lock.insert({m_handle, this});
  else {
    s_lock.erase(m_handle);
    RedrawWindow(m_handle, nullptr, nullptr,
      RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
  }
#endif
}
