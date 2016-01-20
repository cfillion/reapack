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

#include <set>
#include <string>

#include "path.hpp"
#include "database.hpp"

class Package;
class Path;
class Remote;
class Version;

class Registry {
public:
  Registry(const Path &path = Path());

  enum Status {
    Unknown,
    Uninstalled,
    UpdateAvailable,
    UpToDate,
  };

  struct Entry {
    int id; // internal use
    Status status;
    uint64_t version;
  };

  Entry query(Package *) const;
  std::vector<Entry> queryAll(const Remote &) const;
  std::set<Path> getFiles(const Entry &) const;
  void push(Version *, std::vector<Path> *conflicts = nullptr);
  void forget(const Entry &);
  void commit();

  bool addToREAPER(Version *ver, const Path &root);

private:
  void migrate();

  Database m_db;
  Statement *m_insertEntry;
  Statement *m_findEntry;
  Statement *m_allEntries;
  Statement *m_forgetEntry;

  Statement *m_getFiles;
  Statement *m_insertFile;
  Statement *m_clearFiles;
  Statement *m_forgetFiles;

  Statement *m_savepoint;
  Statement *m_release;
  Statement *m_restore;
};

#endif
