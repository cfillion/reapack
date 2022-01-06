/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2022  Christian Fillion
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

Filter::Filter(const std::string &input)
  : m_root(Group::MatchAll)
{
  set(input);
}

void Filter::set(const std::string &input)
{
  m_input = input;
  m_root.clear();

  std::string buf;
  char quote = 0;
  int flags = 0;
  Group *group = &m_root;

  for(size_t i = 0; i < input.size(); ++i) {
    const char c = input[i];

    const bool isStart = buf.empty(),
               isEnd = i+1 == input.size() || input[i+1] == '\x20';

    if((c == '"' || c == '\'') && ((!quote && isStart) || quote == c)) {
      if(quote)
        quote = 0;
      else {
        flags |= Node::LiteralFlag | Node::FullWordFlag;
        quote = c;
      }
      continue;
    }
    else if(c == '\x20') {
      if(quote)
        flags &= ~Node::FullWordFlag;
      else {
        group = group->push(buf, &flags);
        buf.clear();
        continue;
      }
    }
    else if(!quote) {
      if(c == '^' && isStart) {
        flags |= Node::StartAnchorFlag;
        continue;
      }
      else if(c == '$' && isEnd) {
        flags |= Node::EndAnchorFlag;
        continue;
      }
      else if(flags & Node::LiteralFlag) {
        // force-close the token after having parsed a closing quote
        // and only after having parsed all trailing anchors
        group = group->push(buf, &flags);
        buf.clear();
      }
    }

    buf += c;
  }

  group->push(buf, &flags);
}

bool Filter::match(std::vector<std::string> rows) const
{
  for(std::string &str : rows)
    boost::algorithm::to_lower(str);

  return m_root.match(rows);
}

Filter::Group::Group(Type type, int flags, Group *parent)
  : Node(flags), m_parent(parent), m_type(type)
{
}

Filter::Group *Filter::Group::push(const std::string &buf, int *flags)
{
  if(buf.empty())
    return this;

  if(!(*flags & LiteralFlag)) {
    if(buf == "NOT") {
      *flags ^= NotFlag;
      return this;
    }
    else if(buf == "OR") {
      if(m_nodes.empty())
        return this; // no previous token, ignore

      Group *currentOr = dynamic_cast<Group *>(m_nodes.back().get());
      if(currentOr && currentOr->m_type == MatchAny)
        return currentOr;

      auto prev = std::move(m_nodes.back());
      m_nodes.pop_back();

      Group *newGroup = addSubGroup(MatchAny, 0);
      newGroup->m_nodes.push_back(std::move(prev));
      return newGroup;
    }
    else if(buf == "(") {
      Group *newGroup = addSubGroup(MatchAll, *flags);
      *flags = 0;
      return newGroup;
    }
    else if(buf == ")") {
      for(Group *parent = m_parent; parent; parent = parent->m_parent) {
        if(parent->m_type == MatchAll)
          return parent;
      }

      return this;
    }
    else if(pushSynonyms(buf, flags))
      return this;
  }

  m_nodes.push_back(std::make_unique<Token>(buf, *flags));
  *flags = 0;

  Group *group = this;
  while(group->m_type != MatchAll && group->m_parent)
    group = group->m_parent;
  return group;
}

Filter::Group *Filter::Group::addSubGroup(const Type type, const int flags)
{
  auto newGroup = std::make_unique<Group>(type, flags, this);
  Group *ptr = newGroup.get();
  m_nodes.push_back(std::move(newGroup));

  return ptr;
}

bool Filter::Group::pushSynonyms(const std::string &buf, int *flags)
{
  static const std::vector<std::string> synonyms[] {
    { "open", "display", "view", "show", "hide" },
    { "delete", "clear", "remove", "erase" },
    { "insert", "add" },
    { "deselect", "unselect" },
  };

  auto *match = [&]() -> decltype(&*synonyms) {
    for(const auto &synonym : synonyms) {
      for(const auto &word : synonym) {
        if(boost::iequals(buf, word))
          return &synonym;
      }
    }
    return nullptr;
  }();

  if(!match)
    return false;

  Group *notGroup;
  if(*flags & NotFlag) {
    notGroup = addSubGroup(MatchAll, NotFlag);
    *flags ^= NotFlag;
  }
  else
    notGroup = this;

  Group *orGroup = notGroup->addSubGroup(MatchAny, 0);
  if(!(*flags & FullWordFlag))
    orGroup->m_nodes.push_back(std::make_unique<Token>(buf, *flags));
  for(const auto &word : *match)
    orGroup->m_nodes.push_back(std::make_unique<Token>(word, *flags | FullWordFlag));

  *flags = 0;
  return true;
}

bool Filter::Group::match(const std::vector<std::string> &rows) const
{
  for(const auto &node : m_nodes) {
    if(node->match(rows)) {
      if(m_type == MatchAny)
        return true;
    }
    else if(m_type == MatchAll)
      return test(NotFlag);
  }

  return m_type == MatchAll && !test(NotFlag);
}

Filter::Token::Token(const std::string &buf, int flags)
  : Node(flags), m_buf(buf)
{
  boost::algorithm::to_lower(m_buf);
}

bool Filter::Token::match(const std::vector<std::string> &rows) const
{
  const bool isNot = test(NotFlag);
  bool match = false;

  for(const std::string &row : rows) {
    if(matchRow(row) ^ isNot)
      match = true;
    else if(isNot)
      return false;
  }

  return match;
}

bool Filter::Token::matchRow(const std::string &str) const
{
  const size_t pos = str.find(m_buf);

  if(pos == std::string::npos)
    return false;

  const bool isStart = pos == 0, isEnd = pos + m_buf.size() == str.size();

  if(test(StartAnchorFlag) && !isStart)
    return false;
  if(test(EndAnchorFlag) && !isEnd)
    return false;
  if(test(FullWordFlag)) {
    return
      (isStart || !isalnum(str[pos - 1])) &&
      (isEnd || !isalnum(str[pos + m_buf.size()]));
  }

  return true;
}
