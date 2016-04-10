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
#include "package.hpp"

class Package;
class Path;
class Remote;
class Version;

class Registry {
public:
  struct Entry {
    int id;
    std::string remote;
    std::string category;
    std::string package;
    Package::Type type;
    Version version;

    operator bool() const { return id != 0; }
  };

  Registry(const Path &path = {});

  Entry getEntry(const Package *) const;
  std::vector<Entry> getEntries(const std::string &) const;
  std::set<Path> getFiles(const Entry &) const;
  std::string getMainFile(const Entry &) const;
  Entry push(const Version *, std::vector<Path> *conflicts = nullptr);
  void forget(const Entry &);
  void savepoint();
  void restore();
  void release();
  void commit();

private:
  void migrate();

  Database m_db;
  Statement *m_insertEntry;
  Statement *m_updateEntry;
  Statement *m_findEntry;
  Statement *m_allEntries;
  Statement *m_forgetEntry;
  Statement *m_setMainFile;

  Statement *m_getFiles;
  Statement *m_getMainFile;
  Statement *m_insertFile;
  Statement *m_clearFiles;
  Statement *m_forgetFiles;

  size_t m_savePoint;
};

#endif
