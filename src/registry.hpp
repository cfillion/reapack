/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2017  Christian Fillion
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

#include "database.hpp"
#include "package.hpp"
#include "path.hpp"
#include "version.hpp"

#include <set>
#include <string>

class Registry {
public:
  struct Entry {
    typedef int64_t id_t;

    id_t id;
    std::string remote;
    std::string category;
    std::string package;
    std::string description;
    Package::Type type;
    VersionName version;
    std::string author;
    bool pinned;

    operator bool() const { return id > 0; }
    bool operator==(const Entry &o) const { return id == o.id; }
  };

  struct File {
    Path path;
    int sections;
    Package::Type type;

    bool operator<(const File &o) const { return path < o.path; }
  };

  Registry(const Path &path = {});

  Entry getEntry(const Package *) const;
  Entry getOwner(const Path &) const;
  std::vector<Entry> getEntries(const std::string &) const;
  std::vector<File> getFiles(const Entry &) const;
  std::vector<File> getMainFiles(const Entry &) const;
  Entry push(const Version *, std::vector<Path> *conflicts = nullptr);
  void setPinned(const Entry &, bool pinned);
  void forget(const Entry &);

  void savepoint() { m_db.savepoint(); }
  void restore() { m_db.restore(); }
  void commit() { m_db.commit(); }

private:
  void migrate();
  void convertImplicitSections();
  void fillEntry(const Statement *, Entry *) const;

  Database m_db;
  Statement *m_insertEntry;
  Statement *m_updateEntry;
  Statement *m_setPinned;
  Statement *m_findEntry;
  Statement *m_allEntries;
  Statement *m_forgetEntry;
  Statement *m_getOwner;

  Statement *m_getFiles;
  Statement *m_insertFile;
  Statement *m_clearFiles;
  Statement *m_forgetFiles;
};

namespace std {
  template<> struct hash<Registry::Entry> {
    std::size_t operator()(const Registry::Entry &e) const
    {
      return std::hash<Registry::Entry::id_t>()(e.id);
    }
  };
}

#endif
