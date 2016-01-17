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

#include "registry.hpp"

#include "errors.hpp"
#include "package.hpp"
#include "path.hpp"

#include <reaper_plugin_functions.h>

using namespace std;

Registry::Registry(const Path &path)
  : m_db(path.join())
{
  m_db.query(
    "PRAGMA foreign_keys = ON;"

    "CREATE TABLE IF NOT EXISTS entries ("
    "  package TEXT NOT NULL UNIQUE,"
    "  version INTEGER NOT NULL"
    ");"
  );

  m_insertEntry = m_db.prepare(
    "INSERT OR REPLACE INTO entries "
    "VALUES(?, ?);"
  );

  m_findEntry = m_db.prepare(
    "SELECT rowid, version FROM entries "
    "WHERE package = ? "
    "LIMIT 1"
  );
}

void Registry::push(Version *ver)
{
  Package *pkg = ver->package();

  if(!pkg)
    return;

  m_insertEntry->bind(1, hashPackage(pkg).c_str());
  m_insertEntry->bind(2, ver->code());
  m_insertEntry->exec();
}

Registry::QueryResult Registry::query(Package *pkg) const
{
  bool exists = false;
  uint64_t version = 0;

  m_findEntry->bind(1, hashPackage(pkg).c_str());
  m_findEntry->exec([&] {
    version = m_findEntry->uint64Column(1);
    exists = true;
    return false;
  });

  if(!exists)
    return {Uninstalled, 0};

  Version *lastVer = pkg->lastVersion();

  const Status status = version == lastVer->code() ? UpToDate : UpdateAvailable;
  return {status, version};
}

bool Registry::addToREAPER(Version *ver, const Path &root)
{
  if(ver->package()->type() != Package::ScriptType)
    return false;

  Source *src = ver->mainSource();

  if(!src)
    return false;

  enum { MainSection = 0 };
  const string &path = (root + src->targetPath()).join();

  custom_action_register_t ca{MainSection, nullptr, path.c_str()};
  const int id = plugin_register("custom_action", (void *)&ca);

  return id > 0;
}

string Registry::hashPackage(Package *pkg) const
{
  return (pkg->targetPath() + pkg->name()).join('\30');
}
