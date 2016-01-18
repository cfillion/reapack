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
#include "index.hpp"
#include "package.hpp"
#include "path.hpp"

#include <reaper_plugin_functions.h>

using namespace std;

Registry::Registry(const Path &path)
  : m_db(path.join())
{
  migrate();

  m_insertEntry = m_db.prepare(
    "INSERT OR REPLACE INTO entries "
    "VALUES(?, ?, ?, ?);"
  );

  m_findEntry = m_db.prepare(
    "SELECT version FROM entries "
    "WHERE remote = ? AND category = ? AND package = ? "
    "LIMIT 1"
  );

  // lock the database
  m_db.exec("BEGIN EXCLUSIVE TRANSACTION");
}

void Registry::migrate()
{
  switch(m_db.version()) {
  case 0:
    // new database!
    m_db.exec(
      "PRAGMA user_version = 1;"

      "CREATE TABLE entries ("
      "  remote TEXT NOT NULL,"
      "  category TEXT NOT NULL,"
      "  package TEXT NOT NULL,"
      "  version INTEGER NOT NULL,"
      "  UNIQUE(remote, category, package)"
      ");"
    );
    break;
  case 1:
    // current schema version
    break;
  default:
    throw reapack_error("database was created with a newer version of ReaPack");
  }
}

void Registry::push(Version *ver)
{
  Package *pkg = ver->package();
  Category *cat = pkg->category();
  RemoteIndex *ri = cat->index();

  m_insertEntry->bind(1, ri->name());
  m_insertEntry->bind(2, cat->name());
  m_insertEntry->bind(3, pkg->name());
  m_insertEntry->bind(4, ver->code());
  m_insertEntry->exec();
}

Registry::QueryResult Registry::query(Package *pkg) const
{
  bool exists = false;
  uint64_t version = 0;

  Category *cat = pkg->category();
  RemoteIndex *ri = cat->index();

  m_findEntry->bind(1, ri->name());
  m_findEntry->bind(2, cat->name());
  m_findEntry->bind(3, pkg->name());

  m_findEntry->exec([&] {
    version = m_findEntry->uint64Column(0);
    exists = true;
    return false;
  });

  if(!exists)
    return {Uninstalled, 0};

  Version *lastVer = pkg->lastVersion();

  const Status status = version == lastVer->code() ? UpToDate : UpdateAvailable;
  return {status, version};
}

void Registry::commit()
{
  m_db.exec("COMMIT TRANSACTION");
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
