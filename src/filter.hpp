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
  Filter() {}
  Filter(const std::string &input) { set(input); }

  const std::string get() const { return m_input; }
  void set(const std::string &);

  bool match(const std::vector<std::string> &) const;

  Filter &operator=(const std::string &f) { set(f); return *this; }
  bool operator==(const std::string &f) const { return m_input == f; }
  bool operator!=(const std::string &f) const { return !(*this == f); }

private:
  enum Flag {
    StartAnchorFlag = 1<<0,
    EndAnchorFlag = 1<<1,
  };

  struct Token {
    std::string buf;
    int flags;

    bool test(Flag f) const { return (flags & f) != 0; }
    bool match(const std::string &) const;
  };

  std::string m_input;
  std::vector<Token> m_tokens;
};

#endif
