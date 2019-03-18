/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2019  Christian Fillion
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

#include <list>
#include <ostream>
#include <string>

class UseRootPath;

class Path {
public:
  static const Path DATA;
  static const Path CACHE;
  static const Path CONFIG;
  static const Path REGISTRY;

  static const Path &root() { return s_root; }

  Path(const std::string &path = {});

  void append(const std::string &parts, bool traversal = true);
  void append(const Path &other);
  void remove(size_t pos, size_t count);
  void removeLast();
  void clear();

  bool empty() const { return m_parts.empty(); }
  size_t size() const { return m_parts.size(); }
  bool absolute() const { return m_absolute; }

  Path dirname() const;
  std::string front() const;
  std::string basename() const;
  std::string join(bool nativeSeparator = true) const;
  bool startsWith(const Path &) const;

  Path prependRoot() const;
  Path removeRoot() const;

  std::list<std::string>::const_iterator begin() const { return m_parts.begin(); }
  std::list<std::string>::const_iterator end() const { return m_parts.end(); }

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
  bool m_absolute;
};

inline std::ostream &operator<<(std::ostream &os, const Path &p)
{
  return os << p.join();
};

class UseRootPath {
public:
  UseRootPath(const Path &);
  ~UseRootPath();

private:
  Path m_backup;
};

class TempPath {
public:
  TempPath(const Path &target);

  const Path &target() const { return m_target; }
  const Path &temp() const { return m_temp; }

private:
  Path m_target;
  Path m_temp;
};

#endif
