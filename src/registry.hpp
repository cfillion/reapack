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
#include <string>

#include "sqlite.hpp"

class Package;
class Path;
class Version;

class Registry {
public:
  Registry(const std::string &filename = ":memory:");

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

  bool addToREAPER(Version *ver, const Path &root);

  QueryResult query(Package *) const;

private:
  std::string hashPackage(Package *) const;

  SQLite::Database m_db;
  SQLite::Statement *m_insertEntry;
  SQLite::Statement *m_findEntry;
};

#endif
