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

#include "string.hpp"

#ifdef _WIN32
#include <cstdio>

void String::mbtowide(const char *input, UINT codepage)
{
  const int size = MultiByteToWideChar(codepage, 0, input, -1, nullptr, 0) - 1;

  resize(size);
  MultiByteToWideChar(codepage, 0, input, -1, &(*this)[0], size);
}

String::operator std::string() const
{
  const int size = WideCharToMultiByte(CP_UTF8, 0,
    data(), -1, nullptr, 0, nullptr, nullptr) - 1;

  std::string output(size, 0);
  WideCharToMultiByte(CP_UTF8, 0,
    data(), -1, &output[0], size, nullptr, nullptr);

  return output;
}

#define SNPRINTF_DEF(T, impl) \
  int snprintf_auto(T *buf, size_t size, const T *fmt, ...) { \
    va_list va; \
    va_start(va, fmt); \
    int ret = impl(buf, size - 1, fmt, va); \
    buf[size - 1] = 0; \
    va_end(va); \
    return ret; \
  }

// Using _vsnprintf instead of vsnprintf because _vsnwprintf for wide chars
// doesn't ensure null termination and I want to share code between both
// implementations of snpritnf_auto (size vs size-1 would be the only
// difference between the two).

SNPRINTF_DEF(char, _vsnprintf);
SNPRINTF_DEF(wchar_t, _vsnwprintf);

#endif
