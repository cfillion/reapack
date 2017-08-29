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
#include <string>

#ifdef _WIN32
  #include <windows.h>

  typedef wchar_t Char;

  extern int snprintf_auto(char *buf, size_t size, const char *fmt, ...);
  extern int snprintf_auto(wchar_t *buf, size_t size, const wchar_t *fmt, ...);
  #define snprintf(...) snprintf_auto(__VA_ARGS__)
#else
  typedef char Char;
  #define L
#endif

typedef std::basic_string<Char> BasicString;

class String : public BasicString {
public:
  using BasicString::basic_string;

  String() : basic_string() {}
  String(const BasicString &&s) : basic_string(move(s)) {}

#ifdef _WIN32
  String(const std::string &str, UINT codepage = CP_UTF8) : basic_string() { convert(str, codepage); }
  String(const char *str) : basic_string() { convert(str, CP_UTF8); }

  std::string toUtf8() const;
  void convert(const std::string &, UINT codepage);
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
    return std::hash<BasicString>{}(str); }
};

#include <sstream>
typedef std::basic_ostream<Char> BasicStreamO;
typedef std::basic_stringstream<Char> StringStream;
typedef std::basic_ostringstream<Char> StringStreamO;
typedef std::basic_istringstream<Char> StringStreamI;

#include <boost/format/format_fwd.hpp>
typedef boost::basic_format<Char> StringFormat;

#include <regex>
typedef std::basic_regex<Char> Regex;
typedef std::regex_iterator<String::const_iterator> RegexIterator;
typedef std::match_results<String::const_iterator> MatchResults;

#define lengthof(arr) (sizeof(arr) / sizeof(*arr))

#endif
