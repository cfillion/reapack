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

#ifndef REAPACK_VERSION_HPP
#define REAPACK_VERSION_HPP

#include <boost/variant.hpp>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "time.hpp"

class Package;
class Source;
class Path;

class Version {
public:
  typedef std::vector<const Source *> SourceList;
  typedef std::multimap<Path, const Source *> SourceMap;

  Version();
  Version(const std::string &, const Package * = nullptr);
  Version(const Version &, const Package * = nullptr);
  ~Version();

  void parse(const std::string &);
  bool tryParse(const std::string &);

  const std::string &name() const { return m_name; }
  std::string fullName() const;
  size_t size() const { return m_segments.size(); }
  bool isStable() const { return m_stable; }

  const Package *package() const { return m_package; }

  void setAuthor(const std::string &author) { m_author = author; }
  const std::string &author() const { return m_author; }
  std::string displayAuthor() const;

  void setTime(const Time &time) { if(time) m_time = time; }
  const Time &time() const { return m_time; }

  void setChangelog(const std::string &);
  const std::string &changelog() const { return m_changelog; }

  bool addSource(const Source *source);
  const SourceMap &sources() const { return m_sources; }
  const Source *source(size_t) const;
  const SourceList &mainSources() const { return m_mainSources; }

  const std::set<Path> &files() const { return m_files; }

  int compare(const Version &) const;
  bool operator<(const Version &o) const { return compare(o) < 0; }
  bool operator<=(const Version &o) const { return compare(o) <= 0; }
  bool operator>(const Version &o) const { return compare(o) > 0; }
  bool operator>=(const Version &o) const { return compare(o) >= 0; }
  bool operator==(const Version &o) const { return compare(o) == 0; }
  bool operator!=(const Version &o) const { return compare(o) != 0; }

private:
  typedef uint16_t Numeric;
  typedef boost::variant<Numeric, std::string> Segment;

  Segment segment(size_t i) const;

  std::string m_name;
  std::vector<Segment> m_segments;
  bool m_stable;

  std::string m_author;
  std::string m_changelog;
  Time m_time;

  const Package *m_package;
  SourceList m_mainSources;

  SourceMap m_sources;
  std::set<Path> m_files;
};

class VersionPtrCompare {
public:
  bool operator()(const Version *l, const Version *r) const
  {
    return *l < *r;
  }
};

typedef std::set<const Version *, VersionPtrCompare> VersionSet;

#endif
