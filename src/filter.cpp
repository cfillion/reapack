/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2018  Christian Fillion
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

Filter::Filter(const string &input)
  : m_root(Group::MatchAll)
{
  set(input);
}

void Filter::set(const string &input)
{
  enum State { Default, DoubleQuote, SingleQuote };

  m_input = input;
  m_root.clear();

  string buf;
  int flags = 0;
  State state = Default;
  Group *group = &m_root;

  for(const char c : input) {
    if(c == '"' && state != SingleQuote) {
      state = state == Default ? DoubleQuote : Default;
      flags |= Node::QuotedFlag;
      continue;
    }
    else if(c == '\'' && state != DoubleQuote) {
      state = state == Default ? SingleQuote : Default;
      flags |= Node::QuotedFlag;
      continue;
    }
    else if(c == '\x20') {
      if(state == Default) {
        group = group->push(buf, &flags);
        buf.clear();
        continue;
      }
      else
        flags |= Node::PhraseFlag;
    }

    buf += c;
  }

  group->push(buf, &flags);
}

bool Filter::match(vector<string> rows) const
{
  for_each(rows.begin(), rows.end(), [](string &str) { boost::to_lower(str); });
  return m_root.match(rows);
}

Filter::Group::Group(Type type, int flags, Group *parent)
  : Node(flags), m_parent(parent), m_type(type), m_open(true)
{
}

Filter::Group *Filter::Group::push(string buf, int *flags)
{
  if(buf.empty())
    return this;

  if((*flags & QuotedFlag) == 0) {
    if(buf == "NOT") {
      *flags ^= Token::NotFlag;
      return this;
    }
    else if(buf == "OR") {
      if(m_nodes.empty())
        return this;
      else if(m_type == MatchAny) {
        m_open = true;
        return this;
      }

      NodePtr prev = m_nodes.back();
      m_nodes.pop_back();

      auto newGroup = make_shared<Group>(MatchAny, 0, this);
      newGroup->m_nodes.push_back(prev);
      m_nodes.push_back(newGroup);

      return newGroup.get();
    }
    else if(buf == "(") {
      auto newGroup = make_shared<Group>(MatchAll, *flags, this);
      m_nodes.push_back(newGroup);
      *flags = 0;
      return newGroup.get();
    }
    else if(buf == ")") {
      for(Group *parent = this; parent->m_parent; parent = parent->m_parent) {
        if(parent->m_type == MatchAll)
          return parent->m_parent;
      }

      return this;
    }
  }

  if(buf.size() > 1 && buf.front() == '^') {
    *flags |= Node::StartAnchorFlag;
    buf.erase(0, 1); // we need to recheck the size() below, for '$'
  }
  if(buf.size() > 1 && buf.back() == '$') {
    *flags |= Node::EndAnchorFlag;
    buf.pop_back();
  }

  Group *group = m_open ? this : m_parent;

  group->push(make_shared<Token>(buf, *flags));
  *flags = 0;

  return group;
}

void Filter::Group::push(const NodePtr &node)
{
  m_nodes.push_back(node);

  if(m_type == MatchAny)
    m_open = false;
}

bool Filter::Group::match(const vector<string> &rows) const
{
  for(const NodePtr &node : m_nodes) {
    if(node->match(rows)) {
      if(m_type == MatchAny)
        return true;
    }
    else if(m_type == MatchAll)
      return test(NotFlag);
  }

  return m_type == MatchAll && !test(NotFlag);
}

Filter::Token::Token(const string &buf, int flags)
  : Node(flags), m_buf(buf)
{
  boost::to_lower(m_buf);
}

bool Filter::Token::match(const vector<string> &rows) const
{
  const bool isNot = test(NotFlag);
  bool match = false;

  for(const string &row : rows) {
    if(matchRow(row) ^ isNot)
      match = true;
    else if(isNot)
      return false;
  }

  return match;
}

bool Filter::Token::matchRow(const string &str) const
{
  const size_t pos = str.find(m_buf);

  if(pos == string::npos)
    return false;

  const bool isStart = pos == 0, isEnd = pos + m_buf.size() == str.size();

  if(test(StartAnchorFlag) && !isStart)
    return false;
  if(test(EndAnchorFlag) && !isEnd)
    return false;
  if(test(QuotedFlag) && !test(PhraseFlag)) {
    return
      (isStart || !isalnum(str[pos - 1])) &&
      (isEnd || !isalnum(str[pos + m_buf.size()]));
  }

  return true;
}
