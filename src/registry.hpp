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

#ifndef REAPACK_REGISTRY_HPP
#define REAPACK_REGISTRY_HPP

#include <cstdint>
#include <map>
#include <string>

class Package;
class Path;
class Version;

class Registry {
public:
  typedef std::map<std::string, std::string> Map;

  enum Status {
    UpToDate,
    UpdateAvailable,
    Uninstalled,
  };

  struct QueryResult {
    Status status;
    uint64_t versionCode;
  };

  void push(Version *);
  void push(const std::string &key, const std::string &value);

  bool addToREAPER(Version *ver, const Path &root);

  size_t size() const { return m_map.size(); }
  QueryResult query(Package *) const;

  Map::const_iterator begin() const { return m_map.begin(); }
  Map::const_iterator end() const { return m_map.end(); }

private:
  Map m_map;
};

#endif
