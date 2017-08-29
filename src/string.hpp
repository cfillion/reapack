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

#ifndef REAPACK_STRING_HPP
#define REAPACK_STRING_HPP

// This whole file is a huge hack to support unicode on Windows
// MultiByteToWideChar is required to make file path like
// "C:\Users\Test\Downloads\Новая папка" work on Windows...

#include <cstdio>
#include <sstream>
#include <string>

#include <boost/format/format_fwd.hpp>

#ifdef _WIN32
  #include <windows.h>

  typedef wchar_t Char;

  #define AUTOSTR(text) L##text
  #define snprintf(buf, size, ...) _snwprintf(buf, size - 1,  __VA_ARGS__)

  typedef boost::wformat StringFormat;
#else
  typedef char Char;

  #define AUTOSTR(text) text

  typedef boost::format StringFormat;
#endif

#define lengthof(arr) (sizeof(arr) / sizeof(*arr))

typedef std::basic_stringstream<Char> StringStream;
typedef std::basic_ostringstream<Char> StringStreamO;
typedef std::basic_istringstream<Char> StringStreamI;

class String : public std::basic_string<Char> {
public:
  using std::basic_string<Char>::basic_string;

  String() : basic_string() {}
  String(const std::basic_string<Char> &&s) : basic_string(move(s)) {}

#ifdef _WIN32
  String(const std::string &, UINT codepage = CP_UTF8);
  std::string toUtf8() const;
#else
  const String &toUtf8() const { return *this; }
#endif

  template<typename T> static String from(const T v) {
#ifdef _WIN32
    return std::to_wstring(v);
#else
    return std::to_string(v);
#endif
  }
};

template<> struct std::hash<String> {
  size_t operator()(const String &str) const {
    return std::hash<std::basic_string<Char> >{}(str); }
};

#endif
