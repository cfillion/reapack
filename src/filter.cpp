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
#include <unordered_set>

using namespace std;

void Filter::set(const string &input)
{
  enum State { Default, DoubleQuote, SingleQuote };

  Token token{};
  State state = Default;
  m_input = input;

  m_tokens.clear();

  auto push = [&] {
    state = Default;

    size_t size = token.buf.size();

    if(!size)
      return;
    if(size > 1 && token.buf[0] == '^') {
      token.flags |= StartAnchorFlag;
      token.buf.erase(0, 1);
      size--;
    }
    if(size > 1 && token.buf[size - 1] == '$') {
      token.flags |= EndAnchorFlag;
      token.buf.erase(size - 1, 1);
      size--;
    }

    m_tokens.emplace_back(token);
    token = Token();
  };

  for(const char c : input) {
    if(c == '"' && state != SingleQuote) {
      if(state == DoubleQuote)
        state = Default;
      else
        state = DoubleQuote;
    }
    else if(c == '\'' && state != DoubleQuote) {
      if(state == SingleQuote)
        state = Default;
      else
        state = SingleQuote;
    }
    else if(c == '\x20' && state == Default)
      push();
    else
      token.buf += c;
  }

  push();
}

bool Filter::match(const vector<string> &rows) const
{
  unordered_set<const Token *> matches;

  for(const string &str : rows) {
    for(const Token &token : m_tokens) {
      if(token.match(str))
        matches.insert(&token);
    }
  }

  return matches.size() == m_tokens.size();
}

bool Filter::Token::match(const string &str) const
{
  bool match = true;

  if(test(StartAnchorFlag))
    match = match && boost::istarts_with(str, buf);
  if(test(EndAnchorFlag))
    match = match && boost::iends_with(str, buf);

  match = match && boost::icontains(str, buf);

  return match;
}
