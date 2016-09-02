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

#ifndef REAPACK_FILTER_HPP
#define REAPACK_FILTER_HPP

#include <string>
#include <vector>

class Filter {
public:
  Filter(const std::string & = {});

  const std::string get() const { return m_input; }
  void set(const std::string &);

  bool match(const std::vector<std::string> &) const;

  Filter &operator=(const std::string &f) { set(f); return *this; }
  bool operator==(const std::string &f) const { return m_input == f; }
  bool operator!=(const std::string &f) const { return !(*this == f); }

private:
  class Node {
  public:
    virtual bool match(const std::vector<std::string> &) const = 0;
  };

  typedef std::shared_ptr<Node> NodePtr;

  class Group : public Node {
  public:
    enum Type {
      MatchAll,
      MatchAny,
    };

    Group(Type type, Group *parent = nullptr)
      : m_parent(parent), m_type(type), m_open(true) {}

    Group *parent() const { return m_parent; }
    Group *subgroup_any();
    Group *subgroup_all();

    bool empty() const { return m_nodes.empty(); }
    void clear() { m_nodes.clear(); }
    void push(const NodePtr &);

    bool open() const { return m_open; }
    bool match(const std::vector<std::string> &) const override;

  private:
    Group *m_parent;
    Type m_type;
    bool m_open;
    std::vector<NodePtr> m_nodes;
  };

  class Token : public Node {
  public:
    enum Flag {
      StartAnchorFlag = 1<<0,
      EndAnchorFlag   = 1<<1,
      QuotedFlag      = 1<<2,
      NotFlag         = 1<<3,
    };

    Token() : flags(0) {}

    int flags;
    std::string buf;

    bool match(const std::vector<std::string> &) const override;
    bool matchRow(const std::string &) const;
    bool test(Flag f) const { return (flags & f) != 0; }
  };

  std::string m_input;
  Group m_root;
};

#endif
