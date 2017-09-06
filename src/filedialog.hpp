/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2017  Christian Fillion
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

#include "encoding.hpp"

#ifdef _WIN32
#  include <windows.h>
#else
#  include <swell-types.h>
#endif

class Path;

namespace FileDialog {
  auto_string getOpenFileName(HWND, HINSTANCE, const auto_char *title,
    const Path &directory, const auto_char *filters, const auto_char *defext);
  auto_string getSaveFileName(HWND, HINSTANCE, const auto_char *title,
    const Path &directory, const auto_char *filters, const auto_char *defext);
};

#endif
