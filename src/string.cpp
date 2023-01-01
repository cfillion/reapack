/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2023  Christian Fillion
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

#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <cstdarg>
#include <regex>
#include <sstream>

std::string String::format(const char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);
  const int size = vsnprintf(nullptr, 0, fmt, args);
  va_end(args);

  std::string buf(size, 0);

  va_start(args, fmt);
  vsnprintf(&buf[0], size + 1, fmt, args);
  va_end(args);

  return buf;
}

std::string String::indent(const std::string &text)
{
  std::string output;
  std::istringstream input(text);
  std::string line;
  bool first = true;

  while(getline(input, line, '\n')) {
    if(first)
      first = false;
    else
      output += "\r\n";

    boost::algorithm::trim(line);

    if(line.empty())
      continue;

    output += "\x20\x20";
    output += line;
  }

  return output;
}

std::string String::stripRtf(const std::string &rtf)
{
  static const std::regex rtfRules(
    // passthrough for later replacement to \n
    R"((\\line |\\par\}))" "|"
    R"(\\line(\n)\s*)" "|"

    // preserve literal braces
    R"(\\([\{\}]))" "|"

    // hidden groups (strip contents)
    R"(\{\\(?:f\d+|fonttbl|colortbl)[^\{\}]+\})" "|"

    // formatting tags and groups (keep contents)
    R"(\\\w+\s?|\{|\})" "|"

    // newlines and indentation
    R"(\s*\n\s*)"
  );

  std::string text = std::regex_replace(rtf, rtfRules, "$1$2$3");
  boost::algorithm::replace_all(text, "\\line ", "\n");
  boost::algorithm::replace_all(text, "\\par}", "\n\n");
  boost::algorithm::trim(text);

  return text;
}

void String::ImplDetail::imbueStream(std::ostream &stream)
{
  class NumPunct : public std::numpunct<char>
  {
  protected:
    char do_thousands_sep() const override { return ','; }
    std::string do_grouping() const override { return "\3"; }
  };

  stream.imbue(std::locale(std::locale::classic(), new NumPunct));
}
