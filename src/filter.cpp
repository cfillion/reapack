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

  Group *group = &m_root;
  auto token = make_shared<Token>();
  State state = Default;

  auto push = [&] {
    size_t size = token->buf.size();

    if(!size)
      return;

    if(!token->test(Token::QuotedFlag)) {
      if(token->buf == "NOT") {
        token->buf.clear();
        token->flags ^= Token::NotFlag;
        return;
      }
      else if(token->buf == "OR") {
        token->buf.clear();
        if(!group->empty())
          group = group->subgroup_any();
        return;
      }
      else if(token->buf == "(") {
        token->buf.clear();
        group = group->subgroup_all();
        return;
      }
      else if(token->buf == ")") {
        token->buf.clear();
        if(group != &m_root)
          group = group->parent();
        return;
      }
    }

    if(size > 1 && token->buf[0] == '^') {
      token->flags |= Token::StartAnchorFlag;
      token->buf.erase(0, 1);
      size--;
    }
    if(size > 1 && token->buf[size - 1] == '$') {
      token->flags |= Token::EndAnchorFlag;
      token->buf.erase(size - 1, 1);
      size--;
    }

    group->push(token);

    // close previous OR group
    if(!group->open())
      group = group->parent();

    token = make_shared<Token>();
  };

  for(const char c : input) {
    if(c == '"' && state != SingleQuote) {
      if(state == DoubleQuote)
        state = Default;
      else
        state = DoubleQuote;

      token->flags |= Token::QuotedFlag;
    }
    else if(c == '\'' && state != DoubleQuote) {
      if(state == SingleQuote)
        state = Default;
      else
        state = SingleQuote;

      token->flags |= Token::QuotedFlag;
    }
    else if(c == '\x20' && state == Default)
      push();
    else
      token->buf += c;
  }

  push();
}

bool Filter::match(const vector<string> &rows) const
{
  return m_root.match(rows);
}

Filter::Group *Filter::Group::subgroup_any()
{
  if(m_type == Group::MatchAny) {
    m_open = true;
    return this;
  }

  NodePtr prev;
  if(!m_nodes.empty()) {
    prev = m_nodes.back();
    m_nodes.pop_back();
  }

  auto newGroup = make_shared<Group>(Group::MatchAny, this);
  m_nodes.push_back(newGroup);

  if(prev)
    newGroup->m_nodes.push_back(prev);

  return newGroup.get();
}

Filter::Group *Filter::Group::subgroup_all()
{
  // always make a subgroup of this type

  auto newGroup = make_shared<Group>(Group::MatchAll, this);
  m_nodes.push_back(newGroup);
  return newGroup.get();
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
      return false;
  }

  return m_type == MatchAll;
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
  bool match = true;

  if(test(StartAnchorFlag))
    match = match && boost::istarts_with(str, buf);
  if(test(EndAnchorFlag))
    match = match && boost::iends_with(str, buf);

  match = match && boost::icontains(str, buf);

  return match ^ test(NotFlag);
}
