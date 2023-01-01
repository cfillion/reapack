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

#ifndef REAPACK_FILTER_HPP
#define REAPACK_FILTER_HPP

#include <memory>
#include <string>
#include <string_view>
#include <vector>

class Filter {
public:
  Filter(const std::string & = {});
  void set(const std::string &);
  Filter &operator=(const std::string &f) { set(f); return *this; }

  bool match(std::vector<std::string> rows) const;

private:
  class Node {
  public:
    enum Flag {
      StartAnchorFlag = 1<<0,
      EndAnchorFlag   = 1<<1,
      LiteralFlag     = 1<<2,
      NotFlag         = 1<<3,
      FullWordFlag    = 1<<4,
    };

    Node(int flags) : m_flags(flags) {}
    virtual ~Node() = default;

    virtual bool match(const std::vector<std::string> &) const = 0;
    bool test(Flag f) const { return (m_flags & f) != 0; }

  private:
    int m_flags;
  };

  class Group : public Node {
  public:
    enum Type {
      MatchAll,
      MatchAny,
    };

    Group(Type type, int flags = 0, Group *parent = nullptr);
    void clear() { m_nodes.clear(); }
    Group *push(const std::string_view &, int *flags);

    bool match(const std::vector<std::string> &) const override;

  private:
    Group *addSubGroup(Type, int flags);
    bool pushSynonyms(const std::string_view &, int *flags);

    Group *m_parent;
    Type m_type;
    std::vector<std::unique_ptr<Node>> m_nodes;
  };

  class Token : public Node {
  public:
    Token(const std::string_view &buf, int flags);
    bool match(const std::vector<std::string> &) const override;
    bool matchRow(const std::string &) const;

  private:
    std::string_view m_buf;
  };

  std::string m_input;
  Group m_root;
};

#endif
