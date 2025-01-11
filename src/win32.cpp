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

#include "win32.hpp"

#ifdef _WIN32
#  define widen_cstr(cstr) widen(cstr).c_str()
#else
#  include <swell/swell.h>
#  define widen_cstr(cstr) cstr
#endif

#include <vector>

#ifdef _WIN32
std::wstring Win32::widen(const char *input, const UINT codepage)
{
  const int size = MultiByteToWideChar(codepage, 0, input, -1, nullptr, 0) - 1;

  std::wstring output(size, 0);
  MultiByteToWideChar(codepage, 0, input, -1, &output[0], size);

  return output;
}

std::string Win32::narrow(const wchar_t *input)
{
  const int size = WideCharToMultiByte(CP_UTF8, 0,
    input, -1, nullptr, 0, nullptr, nullptr) - 1;

  std::string output(size, 0);
  WideCharToMultiByte(CP_UTF8, 0, input, -1, &output[0], size, nullptr, nullptr);

  return output;
}
#endif

int Win32::messageBox(const HWND handle, const char *text,
  const char *title, const unsigned int buttons)
{
  return MessageBox(handle, widen_cstr(text), widen_cstr(title), buttons);
}

std::string Win32::getWindowText(const HWND handle)
{
  int buffer_len = GetWindowTextLength(handle);
  if(buffer_len)
    ++buffer_len; // null terminator
  else // REAPER <6.09 on non-Windows, before SWELL a34caf91
    buffer_len = 8192;

  std::vector<char_type> buffer(buffer_len, 0);
  GetWindowText(handle, buffer.data(), buffer_len);

  return narrow(buffer.data());
}

void Win32::setWindowText(const HWND handle, const char *text)
{
  SetWindowText(handle, widen_cstr(text));
}

void Win32::shellExecute(const char *what, const char *arg)
{
  ShellExecute(nullptr, L("open"), widen_cstr(what),
    arg ? widen_cstr(arg) : nullptr, nullptr, SW_SHOW);
}

HANDLE Win32::globalCopy(const std::string &text)
{
  // calculate the size in bytes including the null terminator
  const size_t size = (text.size() + 1) * sizeof(char_type);

  HANDLE mem = GlobalAlloc(GMEM_MOVEABLE, size);
  memcpy(GlobalLock(mem), widen_cstr(text.c_str()), size);
  GlobalUnlock(mem);

  return mem;
}

bool Win32::writePrivateProfileString(const char *group, const char *key,
  const char *value, const char *path)
{
  // value can be null for deleting a key
  return WritePrivateProfileString(widen_cstr(group), widen_cstr(key),
    value ? widen_cstr(value) : nullptr, widen_cstr(path));
}

std::string Win32::getPrivateProfileString(const char *group, const char *key,
  const char *fallback, const char *path)
{
  char_type buffer[4096];
  GetPrivateProfileString(widen_cstr(group), widen_cstr(key), widen_cstr(fallback),
    buffer, static_cast<DWORD>(std::size(buffer)), widen_cstr(path));

  return narrow(buffer);
}

unsigned int Win32::getPrivateProfileInt(const char *group, const char *key,
  const unsigned int fallback, const char *path)
{
  return GetPrivateProfileInt(widen_cstr(group), widen_cstr(key),
    fallback, widen_cstr(path));
}
