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

#include <cstdint>
#include <map>
#include <set>
#include <string>

#include "source.hpp"

class Package;

class Version {
public:
  Version(const std::string &, Package * = nullptr);
  ~Version();

  const std::string &name() const { return m_name; }
  std::string fullName() const;
  uint64_t code() const { return m_code; }

  Package *package() const { return m_package; }

  void setAuthor(const std::string &author) { m_author = author; }
  const std::string &author() const { return m_author; }

  void setChangelog(const std::string &);
  const std::string &changelog() const { return m_changelog; }

  void addSource(Source *source);
  const std::multimap<Path, Source *> &sources() const { return m_sources; }
  Source *source(const size_t) const;
  Source *mainSource() const { return m_mainSource; }

  const std::set<Path> &files() const { return m_files; }

  bool operator<(const Version &) const;
  bool operator==(const Version &) const;
  bool operator!=(const Version &) const;

private:
  std::string m_name;
  uint64_t m_code;

  Package *m_package;
  Source *m_mainSource;

  std::string m_author;
  std::string m_changelog;
  std::multimap<Path, Source *> m_sources;
  std::set<Path> m_files;
};

class VersionCompare {
public:
  bool operator()(const Version *l, const Version *r) const
  {
    return *l < *r;
  }
};

typedef std::set<Version *, VersionCompare> VersionSet;

#endif
