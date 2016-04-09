/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2016  Christian Fillion
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

#include "filter.hpp"

#include <boost/algorithm/string.hpp>

using namespace std;

void Filter::set(const string &input)
{
  enum State { Default, DoubleQuote, SingleQuote };

  string buf;
  State state = Default;
  m_input = input;

  m_words.clear();

  auto push = [&] {
    state = Default;

    if(!buf.empty()) {
      m_words.push_back(buf);
      buf.clear();
    }
  };

  for(const char c : input) {
    if(c == '"' && state != SingleQuote) {
      if(state == DoubleQuote)
        push();
      else
        state = DoubleQuote;
    }
    else if(c == '\'' && state != DoubleQuote) {
      if(state == SingleQuote)
        push();
      else
        state = SingleQuote;
    }
    else if(c == '\x20' && state == Default)
      push();
    else
      buf += c;
  }

  push();
}

bool Filter::match(const string &str) const
{
  for(const string &word : m_words) {
    if(!boost::icontains(str, word))
      return false;
  }

  return true;
}
