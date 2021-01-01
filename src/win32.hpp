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

#ifndef REAPACK_WIN32_HPP
#define REAPACK_WIN32_HPP

// Utility wrappers around the Windows Wide API

#include <string>

#ifdef _WIN32
#  define L(str) L##str
#  include <windows.h>
#else
#  define L(str) str
#  include <swell/swell-types.h>
#endif

namespace Win32 {
#ifdef _WIN32
  typedef wchar_t char_type;

  std::wstring widen(const char *, UINT codepage = CP_UTF8);
  inline std::wstring widen(const std::string &str, UINT codepage = CP_UTF8)
  { return widen(str.c_str(), codepage); }

  std::string narrow(const wchar_t *);
  inline std::string narrow(const std::wstring &str) { return narrow(str.c_str()); }

  inline std::string ansi2utf8(const char *str) { return narrow(widen(str, CP_ACP)); }
  inline std::string ansi2utf8(const std::string &str) { return ansi2utf8(str.c_str()); }
#else
  typedef char char_type;

  inline std::string widen(const char *s, UINT = 0) { return s; }
  inline std::string widen(const std::string &s) { return s; }

  inline std::string narrow(const char *s) { return s; }
  inline std::string narrow(const std::string &s) { return s; }
#endif

  int messageBox(HWND, const char *text, const char *title, unsigned int buttons);
  void setWindowText(HWND handle, const char *text);
  std::string getWindowText(HWND handle);
  void shellExecute(const char *what, const char *arg = nullptr);
  HANDLE globalCopy(const std::string &);

  bool writePrivateProfileString(const char *g, const char *k, const char *v, const char *p);
  std::string getPrivateProfileString(const char *g, const char *k, const char *v, const char *p);
  unsigned int getPrivateProfileInt(const char *g, const char *k, unsigned int f, const char *p);
};

#endif
