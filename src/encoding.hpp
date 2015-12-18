/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015  Christian Fillion
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

#ifndef REAPACK_ENCODING_HPP
#define REAPACK_ENCODING_HPP

// This whole file is a huge hack to support unicode on Windows
// MultiByteToWideChar is required to make file path like
// "C:\Users\Test\Downloads\Новая папка" work on Windows...

#include <string>

#ifdef _WIN32

typedef wchar_t auto_char;
typedef std::wstring auto_string;

#define AUTO_STR(text) L##text
#define to_autostring std::to_wstring
auto_string make_autostring(const std::string &);

#else

typedef char auto_char;
typedef std::string auto_string;

#define AUTO_STR(text) text
#define to_autostring std::to_string
#define make_autostring(string) string

#endif

#endif
