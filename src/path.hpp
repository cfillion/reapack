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

#ifndef REAPACK_PATH_HPP
#define REAPACK_PATH_HPP

#include <string>
#include <list>

class Path {
public:
  Path(const std::string &path = std::string());

  void prepend(const std::string &part);
  void append(const std::string &part);
  void removeLast();
  void clear();

  bool empty() const { return m_parts.empty(); }
  size_t size() const { return m_parts.size(); }

  std::string basename() const;
  std::string dirname() const { return join(true, 0); }
  std::string join(const char sep = 0) const { return join(false, sep); }

  bool operator==(const Path &) const;
  bool operator!=(const Path &) const;
  bool operator<(const Path &) const;
  Path operator+(const std::string &) const;
  Path operator+(const Path &) const;
  std::string &operator[](const size_t index);
  const std::string &operator[](const size_t index) const;

private:
  std::string join(const bool, const char) const;
  const std::string &at(const size_t) const;

  std::list<std::string> m_parts;
};

#endif
