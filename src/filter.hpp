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

#ifndef REAPACK_FILTER_HPP
#define REAPACK_FILTER_HPP

#include <memory>
#include <vector>

#include "string.hpp"

class Filter {
public:
  Filter(const String & = {});

  const String get() const { return m_input; }
  void set(const String &);

  bool match(std::vector<String> rows) const;

  Filter &operator=(const String &f) { set(f); return *this; }
  bool operator==(const String &f) const { return m_input == f; }
  bool operator!=(const String &f) const { return !(*this == f); }

private:
  class Node {
  public:
    enum Flag {
      StartAnchorFlag = 1<<0,
      EndAnchorFlag   = 1<<1,
      QuotedFlag      = 1<<2,
      NotFlag         = 1<<3,
      PhraseFlag      = 1<<4,
    };

    Node(int flags) : m_flags(flags) {}

    virtual bool match(const std::vector<String> &) const = 0;
    bool test(Flag f) const { return (m_flags & f) != 0; }

  private:
    int m_flags;
  };

  typedef std::shared_ptr<Node> NodePtr;

  class Group : public Node {
  public:
    enum Type {
      MatchAll,
      MatchAny,
    };

    Group(Type type, int flags = 0, Group *parent = nullptr);
    void clear() { m_nodes.clear(); }
    Group *push(String, int *flags);
    bool match(const std::vector<String> &) const override;

  private:
    void push(const NodePtr &);

    Group *m_parent;
    Type m_type;
    bool m_open;
    std::vector<NodePtr> m_nodes;
  };

  class Token : public Node {
  public:
    Token(const String &buf, int flags);
    bool match(const std::vector<String> &) const override;
    bool matchRow(const String &) const;

  private:
    String m_buf;
  };

  String m_input;
  Group m_root;
};

#endif
