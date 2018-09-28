/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2018  Christian Fillion
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

#ifndef REAPACK_VERSION_HPP
#define REAPACK_VERSION_HPP

#include "time.hpp"

#include <cstdint>
#include <map>
#include <set>
#include <variant>
#include <vector>

class Package;
class Path;
class Source;

class VersionName {
public:
  VersionName();
  VersionName(const std::string &);
  VersionName(const VersionName &);

  void parse(const std::string &);
  bool tryParse(const std::string &, std::string *errorOut = nullptr);

  size_t size() const { return m_segments.size(); }
  bool isStable() const { return m_stable; }
  const std::string &toString() const { return m_string; }

  int compare(const VersionName &) const;
  bool operator<(const VersionName &o) const { return compare(o) < 0; }
  bool operator<=(const VersionName &o) const { return compare(o) <= 0; }
  bool operator>(const VersionName &o) const { return compare(o) > 0; }
  bool operator>=(const VersionName &o) const { return compare(o) >= 0; }
  bool operator==(const VersionName &o) const { return compare(o) == 0; }
  bool operator!=(const VersionName &o) const { return compare(o) != 0; }

private:
  typedef uint16_t Numeric;
  typedef std::variant<Numeric, std::string> Segment;

  Segment segment(size_t i) const;

  std::string m_string;
  std::vector<Segment> m_segments;
  bool m_stable;
};

class Version {
public:
  static std::string displayAuthor(const std::string &name);

  Version(const std::string &, const Package *);
  ~Version();

  const VersionName &name() const { return m_name; }
  const Package *package() const { return m_package; }
  std::string fullName() const;

  void setAuthor(const std::string &author) { m_author = author; }
  const std::string &author() const { return m_author; }
  std::string displayAuthor() const { return displayAuthor(m_author); }

  void setTime(const Time &time) { if(time) m_time = time; }
  const Time &time() const { return m_time; }

  void setChangelog(const std::string &cl) { m_changelog = cl; }
  const std::string &changelog() const { return m_changelog; }

  bool addSource(const Source *source);
  const auto &sources() const { return m_sources; }
  const Source *source(size_t i) const { return m_sources[i]; }

  const std::set<Path> &files() const { return m_files; }

private:
  VersionName m_name;
  std::string m_author;
  std::string m_changelog;
  Time m_time;
  const Package *m_package;
  std::vector<const Source *> m_sources;
  std::set<Path> m_files;
};

std::ostream &operator<<(std::ostream &, const Version &);

#endif
