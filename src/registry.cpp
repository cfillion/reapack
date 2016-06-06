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

#include <sqlite3.h>

using namespace std;

Registry::Registry(const Path &path)
  : m_db(path.join()), m_savePoint(0)
{
  migrate();

  // entry queries
  m_insertEntry = m_db.prepare(
    "INSERT INTO entries(remote, category, package, type, version, author)"
    "VALUES(?, ?, ?, ?, ?, ?);"
  );

  m_updateEntry = m_db.prepare(
    "UPDATE entries SET type = ?, version = ?, author = ? WHERE id = ?"
  );

  m_setPinned = m_db.prepare("UPDATE entries SET pinned = ? WHERE id = ?");

  m_findEntry = m_db.prepare(
    "SELECT id, remote, category, package, type, version, author, pinned "
    "FROM entries WHERE remote = ? AND category = ? AND package = ? LIMIT 1"
  );

  m_allEntries = m_db.prepare(
    "SELECT id, category, package, type, version, author, pinned "
    "FROM entries WHERE remote = ?"
  );
  m_forgetEntry = m_db.prepare("DELETE FROM entries WHERE id = ?");

  // file queries
  m_getFiles = m_db.prepare("SELECT path FROM files WHERE entry = ?");
  m_getMainFiles = m_db.prepare(
    "SELECT path, type FROM files WHERE main = 1 AND entry = ?"
  );
  m_insertFile = m_db.prepare("INSERT INTO files VALUES(NULL, ?, ?, ?, ?)");
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
      "CREATE TABLE entries ("
      "  id INTEGER PRIMARY KEY,"
      "  remote TEXT NOT NULL,"
      "  category TEXT NOT NULL,"
      "  package TEXT NOT NULL,"
      "  type INTEGER NOT NULL,"
      "  version TEXT NOT NULL,"
      "  author TEXT NOT NULL,"
      "  pinned INTEGER DEFAULT 0,"
      "  UNIQUE(remote, category, package)"
      ");"

      "CREATE TABLE files ("
      "  id INTEGER PRIMARY KEY,"
      "  entry INTEGER NOT NULL,"
      "  path TEXT UNIQUE NOT NULL,"
      "  main INTEGER NOT NULL,"
      "  type INTEGER,"
      "  FOREIGN KEY(entry) REFERENCES entries(id)"
      ");"
    );
    break;
  case 1:
    m_db.exec("ALTER TABLE entries ADD COLUMN pinned INTEGER NOT NULL DEFAULT 0;");
  case 2:
    m_db.exec("ALTER TABLE files ADD COLUMN type INTEGER NOT NULL DEFAULT 0;");
  case 3:
    // current schema version
    break;
  default:
    throw reapack_error(
      "The package registry was created by a newer version of ReaPack");
  }

  m_db.exec("PRAGMA user_version = 3");
  m_db.commit();
}

auto Registry::push(const Version *ver, vector<Path> *conflicts) -> Entry
{
  savepoint();

  bool hasConflicts = false;

  const Package *pkg = ver->package();
  const Category *cat = pkg->category();
  const Index *ri = cat->index();

  m_clearFiles->bind(1, ri->name());
  m_clearFiles->bind(2, cat->name());
  m_clearFiles->bind(3, pkg->name());
  m_clearFiles->exec();

  int entryId = getEntry(ver->package()).id;

  // register version
  if(entryId) {
    m_updateEntry->bind(1, pkg->type());
    m_updateEntry->bind(2, ver->name());
    m_updateEntry->bind(3, ver->author());
    m_updateEntry->bind(4, entryId);
    m_updateEntry->exec();
  }
  else {
    m_insertEntry->bind(1, ri->name());
    m_insertEntry->bind(2, cat->name());
    m_insertEntry->bind(3, pkg->name());
    m_insertEntry->bind(4, pkg->type());
    m_insertEntry->bind(5, ver->name());
    m_insertEntry->bind(6, ver->author());
    m_insertEntry->exec();

    entryId = m_db.lastInsertId();
  }

  // register files
  const auto &sources = ver->sources();
  for(auto it = sources.begin(); it != sources.end();) {
    const Path &path = it->first;
    const Source *src = it->second;

    m_insertFile->bind(1, entryId);
    m_insertFile->bind(2, path.join('/'));
    m_insertFile->bind(3, src->isMain());
    m_insertFile->bind(4, src->typeOverride());

    try {
      m_insertFile->exec();
    }
    catch(const reapack_error &) {
      if(conflicts && m_db.errorCode() == SQLITE_CONSTRAINT_UNIQUE) {
        hasConflicts = true;
        conflicts->push_back(path);
      }
      else {
        restore();
        throw;
      }
    }

    // skip duplicate files
    do { it++; } while(it != sources.end() && path == it->first);
  }

  if(hasConflicts) {
    restore();
    return {};
  }
  else {
    release();
    return {entryId, ri->name(), cat->name(), pkg->name(), pkg->type(), *ver};
  }
}

void Registry::setPinned(const Entry &entry, const bool pinned)
{
  m_setPinned->bind(1, pinned);
  m_setPinned->bind(2, entry.id);
  m_setPinned->exec();
}

auto Registry::getEntry(const Package *pkg) const -> Entry
{
  Entry entry{};

  const Category *cat = pkg->category();
  const Index *ri = cat->index();

  m_findEntry->bind(1, ri->name());
  m_findEntry->bind(2, cat->name());
  m_findEntry->bind(3, pkg->name());

  m_findEntry->exec([&] {
    int col = 0;

    entry.id = m_findEntry->intColumn(col++);
    entry.remote = m_findEntry->stringColumn(col++);
    entry.category = m_findEntry->stringColumn(col++);
    entry.package = m_findEntry->stringColumn(col++);
    entry.type = static_cast<Package::Type>(m_findEntry->intColumn(col++));
    entry.version.tryParse(m_findEntry->stringColumn(col++));
    entry.version.setAuthor(m_findEntry->stringColumn(col++));
    entry.pinned = m_findEntry->intColumn(col++) != 0;

    return false;
  });

  return entry;
}

auto Registry::getEntries(const string &remoteName) const -> vector<Entry>
{
  vector<Registry::Entry> list;

  m_allEntries->bind(1, remoteName);
  m_allEntries->exec([&] {
    int col = 0;

    Entry entry{};
    entry.id = m_allEntries->intColumn(col++);
    entry.remote = remoteName;
    entry.category = m_allEntries->stringColumn(col++);
    entry.package = m_allEntries->stringColumn(col++);
    entry.type = static_cast<Package::Type>(m_allEntries->intColumn(col++));
    entry.version.tryParse(m_allEntries->stringColumn(col++));
    entry.version.setAuthor(m_allEntries->stringColumn(col++));
    entry.pinned = m_allEntries->intColumn(col++) != 0;

    list.push_back(entry);

    return true;
  });

  return list;
}

set<Path> Registry::getFiles(const Entry &entry) const
{
  if(!entry) // skip processing for new packages
    return {};

  set<Path> list;

  m_getFiles->bind(1, entry.id);
  m_getFiles->exec([&] {
    list.insert(m_getFiles->stringColumn(0));
    return true;
  });

  return list;
}

auto Registry::getMainFiles(const Entry &entry) const -> vector<File>
{
  if(!entry)
    return {};

  vector<File> files;

  m_getMainFiles->bind(1, entry.id);
  m_getMainFiles->exec([&] {
    File file{m_getMainFiles->stringColumn(0)};
    file.type = static_cast<Package::Type>(m_getMainFiles->intColumn(1));

    if(!file.type) // < v1.0rc2
      file.type = entry.type;

    files.push_back(file);
    return true;
  });

  return files;
}

void Registry::forget(const Entry &entry)
{
  m_forgetFiles->bind(1, entry.id);
  m_forgetFiles->exec();

  m_forgetEntry->bind(1, entry.id);
  m_forgetEntry->exec();
}

void Registry::savepoint()
{
  char sql[64] = {};
  sprintf(sql, "SAVEPOINT sp%zu", m_savePoint++);

  m_db.exec(sql);
}

void Registry::restore()
{
  char sql[64] = {};
  sprintf(sql, "ROLLBACK TO SAVEPOINT sp%zu", --m_savePoint);

  m_db.exec(sql);
}

void Registry::release()
{
  char sql[64] = {};
  sprintf(sql, "RELEASE SAVEPOINT sp%zu", --m_savePoint);

  m_db.exec(sql);
}

void Registry::commit()
{
  m_db.commit();
}
