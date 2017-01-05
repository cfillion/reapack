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
      if(state == DoubleQuote)
        state = Default;
      else
        state = DoubleQuote;

      flags |= Node::QuotedFlag;
    }
    else if(c == '\'' && state != DoubleQuote) {
      if(state == SingleQuote)
        state = Default;
      else
        state = SingleQuote;

      flags |= Node::QuotedFlag;
    }
    else if(c == '\x20' && state == Default) {
      group = group->push(buf, &flags);
      buf.clear();
    }
    else
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
  size_t size = buf.size();

  if(!size)
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

      NodePtr prev;
      if(!m_nodes.empty()) {
        prev = m_nodes.back();
        m_nodes.pop_back();
      }

      auto newGroup = make_shared<Group>(MatchAny, 0, this);
      m_nodes.push_back(newGroup);

      if(prev)
        newGroup->m_nodes.push_back(prev);

      return newGroup.get();
    }
    else if(buf == "(") {
      auto newGroup = make_shared<Group>(MatchAll, *flags, this);
      m_nodes.push_back(newGroup);
      *flags = 0;
      return newGroup.get();
    }
    else if(buf == ")") {
      Group *parent = this;

      while(parent->m_parent) {
        const Type type = parent->m_type;
        parent = parent->m_parent;

        if(type == MatchAll)
          break;
      }

      return parent;
    }
  }

  if(size > 1 && buf[0] == '^') {
    *flags |= Node::StartAnchorFlag;
    buf.erase(0, 1);
    size--;
  }
  if(size > 1 && buf[size - 1] == '$') {
    *flags |= Node::EndAnchorFlag;
    buf.erase(size - 1, 1);
    size--;
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
  bool match = false;

  for(const string &row : rows) {
    if(matchRow(row))
      match = true;
    else if(test(NotFlag))
      return false;
  }

  return match;
}

bool Filter::Token::matchRow(const string &str) const
{
  const size_t pos = str.find(m_buf);
  const bool fail = test(NotFlag);

  if(test(StartAnchorFlag) && pos != 0)
    return fail;
  if(test(EndAnchorFlag) && pos + m_buf.size() != str.size())
    return fail;

  return (pos != string::npos) ^ fail;
}
