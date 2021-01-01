/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2021  Christian Fillion
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

const Platform::Enum Platform::Current = Platform::
#ifdef __APPLE__
#  ifdef __x86_64__
  Darwin_x86_64
#  elif  __i386__
  Darwin_i386
#  elif  __arm64__
  Darwin_arm64
#  else
  Unknown
#  endif

#elif __linux__
#  ifdef __x86_64__
  Linux_x86_64
#  elif  __i686__
  Linux_i686
#  elif  __ARM_ARCH_7A__
  Linux_armv7l
#  elif  __aarch64__
  Linux_aarch64
#  else
  Unknown
#  endif

#elif _WIN32
#  ifdef _WIN64
  Windows_x64
#  else
  Windows_x86
#  endif

#else
  Unknown
#endif
;

static_assert(Platform::Current != Platform::Unknown,
  "The current operating system or architecture is not supported.");

auto Platform::parse(const char *platform) -> Enum
{
  constexpr std::pair<const char *, Enum> map[] {
    { "all",           Generic       },

    { "darwin",        Darwin_Any    },
    { "darwin32",      Darwin_i386   },
    { "darwin64",      Darwin_x86_64 },
    { "darwin-arm64",  Darwin_arm64  },

    { "linux",         Linux_Any     },
    { "linux32",       Linux_i686    },
    { "linux64",       Linux_x86_64  },
    { "linux-armv7l",  Linux_armv7l  },
    { "linux-aarch64", Linux_aarch64 },

    { "windows",       Windows_Any   },
    { "win32",         Windows_x86   },
    { "win64",         Windows_x64   },
  };

  for(auto &[key, value] : map) {
    if(!strcmp(platform, key))
      return value;
  }

  return Unknown;
}

bool Platform::test() const
{
  return (m_value & Current) != Unknown;
}
