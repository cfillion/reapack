/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2024  Christian Fillion
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

#ifndef REAPACK_FILEBROWSE_HPP
#define REAPACK_FILEBROWSE_HPP

#include "win32.hpp"

class Path;

namespace FileDialog {
  std::string getOpenFileName(HWND, HINSTANCE, const char *title,
    const Path &directory, const Win32::char_type *filters, const Win32::char_type *defext);
  std::string getSaveFileName(HWND, HINSTANCE, const char *title,
    const Path &directory, const Win32::char_type *filters, const Win32::char_type *defext);
};

#endif
