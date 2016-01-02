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

#include "encoding.hpp"

#ifdef _WIN32
#include <windows.h>

auto_string make_autostring(const std::string &input)
{
  const int size = MultiByteToWideChar(CP_UTF8, 0, &input[0], -1, nullptr, 0);

  auto_string output(size, 0);
  MultiByteToWideChar(CP_UTF8, 0, &input[0], -1, &output[0], size);

  return output;
}

std::string from_autostring(const auto_string &input)
{
  const int size = WideCharToMultiByte(CP_UTF8, 0,
    &input[0], -1, nullptr, 0, nullptr, nullptr);

  std::string output(size, 0);
  WideCharToMultiByte(CP_UTF8, 0,
    &input[0], -1, &output[0], size, nullptr, nullptr);

  return output;
}
#endif
