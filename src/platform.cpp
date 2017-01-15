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

#include "platform.hpp"

#include <cstring>

auto Platform::parse(const char *platform) -> Enum
{
  if(!strcmp(platform, "all"))
    return GenericPlatform;
  else if(!strcmp(platform, "windows"))
    return WindowsPlatform;
  else if(!strcmp(platform, "win32"))
    return Win32Platform;
  else if(!strcmp(platform, "win64"))
    return Win64Platform;
  else if(!strcmp(platform, "darwin"))
    return DarwinPlatform;
  else if(!strcmp(platform, "darwin32"))
    return Darwin32Platform;
  else if(!strcmp(platform, "darwin64"))
    return Darwin64Platform;
  else
    return UnknownPlatform;
}

bool Platform::test() const
{
  switch(m_value) {
  case GenericPlatform:
#ifdef __APPLE__
  case DarwinPlatform:
#ifdef __x86_64__
  case Darwin64Platform:
#else
  case Darwin32Platform:
#endif

#elif _WIN32
  case WindowsPlatform:
#ifdef _WIN64
  case Win64Platform:
#else
  case Win32Platform:
#endif
#endif
    return true;
  default:
    return false;
  }
}
