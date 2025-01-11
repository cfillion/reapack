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

#include "dialog.hpp"

#include <AppKit/NSTextView.h>
#include <swell/swell.h>

bool Dialog::isTextEditUnderMouse() const
{
  POINT p;
  GetCursorPos(&p);
  const HWND ctrl = WindowFromPoint(p);

  return ctrl && [static_cast<id>(ctrl) isKindOfClass:[NSTextView class]];
}
