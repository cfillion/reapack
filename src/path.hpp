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

class UseRootPath;

class Path {
public:
  static Path DATA;
  static Path CACHE;
  static Path CONFIG;
  static Path REGISTRY;

  static Path prefixRoot(const Path &p) { return s_root + p; }
  static Path prefixRoot(const std::string &p) { return s_root + p; }

  Path(const std::string &path = std::string());

  void prepend(const std::string &part);
  void append(const std::string &part, bool traversal = true);
  void prepend(const Path &other);
  void append(const Path &other);
  void removeFirst();
  void removeLast();
  void clear();

  bool empty() const { return m_parts.empty(); }
  size_t size() const { return m_parts.size(); }

  std::string basename() const;
  Path dirname() const;
  std::string join(const char sep = 0) const;
  std::string first() const;
  std::string last() const;

  bool operator==(const Path &) const;
  bool operator!=(const Path &) const;
  bool operator<(const Path &) const;
  Path operator+(const std::string &) const;
  Path operator+(const Path &) const;
  const Path &operator+=(const std::string &);
  const Path &operator+=(const Path &);
  std::string &operator[](size_t);
  const std::string &operator[](size_t) const;

private:
  static Path s_root;
  friend UseRootPath;

  const std::string &at(size_t) const;

  std::list<std::string> m_parts;
};

class UseRootPath {
public:
  UseRootPath(const std::string &);
  ~UseRootPath();

private:
  Path m_backup;
};

#endif
