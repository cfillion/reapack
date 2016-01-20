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
#include "remote.hpp"

#include <reaper_plugin_functions.h>

using namespace std;

Registry::Registry(const Path &path)
  : m_db(path.join())
{
  migrate();

  // entry queries
  m_insertEntry = m_db.prepare(
    "INSERT OR REPLACE INTO entries "
    "VALUES(NULL, ?, ?, ?, ?);"
  );

  m_findEntry = m_db.prepare(
    "SELECT id, version FROM entries "
    "WHERE remote = ? AND category = ? AND package = ? "
    "LIMIT 1"
  );

  m_allEntries = m_db.prepare("SELECT id, version FROM entries WHERE remote = ?");
  m_forgetEntry = m_db.prepare("DELETE FROM entries WHERE id = ?");

  // file queries
  m_getFiles = m_db.prepare("SELECT path FROM files WHERE entry = ?");
  m_insertFile = m_db.prepare("INSERT INTO files VALUES(NULL, ?, ?)");
  m_clearFiles = m_db.prepare(
    "DELETE FROM files WHERE entry = ("
    "  SELECT id FROM entries WHERE remote = ? AND category = ? AND package = ?"
    ")"
  );
  m_forgetFiles = m_db.prepare("DELETE FROM files WHERE entry = ?");

  // lock the database
  m_db.begin();
}

void Registry::migrate()
{
  m_db.begin();

  switch(m_db.version()) {
  case 0:
    // new database!
    m_db.exec(
      "PRAGMA user_version = 1;"

      "CREATE TABLE entries ("
      "  id INTEGER PRIMARY KEY,"
      "  remote TEXT NOT NULL,"
      "  category TEXT NOT NULL,"
      "  package TEXT NOT NULL,"
      "  version INTEGER NOT NULL,"
      "  UNIQUE(remote, category, package)"
      ");"

      "CREATE TABLE files ("
      "  id INTEGER PRIMARY KEY,"
      "  entry INTEGER NOT NULL,"
      "  path TEXT UNIQUE NOT NULL,"
      "  FOREIGN KEY(entry) REFERENCES entries(id)"
      ");"
    );
    break;
  case 1:
    // current schema version
    break;
  default:
    throw reapack_error("database was created with a newer version of ReaPack");
  }

  m_db.commit();
}

void Registry::push(Version *ver)
{
  Package *pkg = ver->package();
  Category *cat = pkg->category();
  RemoteIndex *ri = cat->index();

  m_clearFiles->bind(1, ri->name());
  m_clearFiles->bind(2, cat->name());
  m_clearFiles->bind(3, pkg->name());
  m_clearFiles->exec();

  m_insertEntry->bind(1, ri->name());
  m_insertEntry->bind(2, cat->name());
  m_insertEntry->bind(3, pkg->name());
  m_insertEntry->bind(4, ver->code());
  m_insertEntry->exec();

  const uint64_t entryId = m_db.lastInsertId();

  for(const Path &path : ver->files()) {
    m_insertFile->bind(1, entryId);
    m_insertFile->bind(2, path.join('/'));
    m_insertFile->exec();
  }
}

Registry::Entry Registry::query(Package *pkg) const
{
  int id = 0;
  uint64_t version = 0;

  Category *cat = pkg->category();
  RemoteIndex *ri = cat->index();

  m_findEntry->bind(1, ri->name());
  m_findEntry->bind(2, cat->name());
  m_findEntry->bind(3, pkg->name());

  m_findEntry->exec([&] {
    id = m_findEntry->intColumn(0);
    version = m_findEntry->uint64Column(1);
    return false;
  });

  if(!id)
    return {id, Uninstalled, 0};

  Version *lastVer = pkg->lastVersion();

  const Status status = version == lastVer->code() ? UpToDate : UpdateAvailable;
  return {id, status, version};
}

vector<Registry::Entry> Registry::queryAll(const Remote &remote) const
{
  vector<Registry::Entry> list;

  m_allEntries->bind(1, remote.name());
  m_allEntries->exec([&] {
    const int id = m_allEntries->intColumn(0);
    const uint64_t version = m_allEntries->uint64Column(1);

    list.push_back({id, Unknown, version});
    return true;
  });

  return list;
}

set<Path> Registry::getFiles(const Entry &qr) const
{
  if(!qr.id) // skip processing for new packages
    return {};

  set<Path> list;

  m_getFiles->bind(1, qr.id);
  m_getFiles->exec([&] {
    list.insert(m_getFiles->stringColumn(0));
    return true;
  });

  return list;
}

void Registry::forget(const Entry &qr)
{
  m_forgetFiles->bind(1, qr.id);
  m_forgetFiles->exec();

  m_forgetEntry->bind(1, qr.id);
  m_forgetEntry->exec();
}

void Registry::commit()
{
  m_db.commit();
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
