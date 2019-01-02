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

#include "platform.hpp"

#include <cstring>
#include <utility>

auto Platform::parse(const char *platform) -> Enum
{
  const std::pair<const char *, Enum> map[]{
    {"all",      GenericPlatform},

    {"windows",  WindowsPlatform},
    {"win32",    Win32Platform},
    {"win64",    Win64Platform},

    {"darwin",   DarwinPlatform},
    {"darwin32", Darwin32Platform},
    {"darwin64", Darwin64Platform},

    {"linux",    LinuxPlatform},
    {"linux32",  Linux32Platform},
    {"linux64",  Linux64Platform},
  };

  for(auto &[key, value] : map) {
    if(!strcmp(platform, key))
      return value;
  }

  return UnknownPlatform;
}

bool Platform::test() const
{
  switch(m_value) {
  case GenericPlatform:

#ifdef __APPLE__
  case DarwinPlatform:
#  ifdef __x86_64__
  case Darwin64Platform:
#  else
  case Darwin32Platform:
#  endif

#elif __linux__
  case LinuxPlatform:
#  ifdef __x86_64__
  case Linux64Platform:
#  else
  case Linux32Platform:
#  endif

#elif _WIN32
  case WindowsPlatform:
#  ifdef _WIN64
  case Win64Platform:
#  else
  case Win32Platform:
#  endif
#endif
    return true;
  default:
    return false;
  }
}
