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

#include <boost/algorithm/string/trim.hpp>
#include <cstdarg>
#include <sstream>

using namespace std;

string String::format(const char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);
  const int size = vsnprintf(nullptr, 0, fmt, args);
  va_end(args);

  string buf(size, 0);

  va_start(args, fmt);
  vsnprintf(&buf[0], size + 1, fmt, args);
  va_end(args);

  return buf;
}

string String::indent(const string &text)
{
  ostringstream output;
  istringstream input(text);
  string line;
  bool first = true;

  while(getline(input, line, '\n')) {
    if(first)
      first = false;
    else
      output << "\r\n";

    boost::algorithm::trim(line);

    if(!line.empty())
      output << "\x20\x20" << line;
  }

  return output.str();
}
