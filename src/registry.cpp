/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2018  Christian Fillion
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

#include <algorithm>

#include <sqlite3.h>

using namespace std;

Registry::Registry(const Path &path)
  : m_db(path.join())
{
  migrate();

  // entry queries
  m_insertEntry = m_db.prepare(
    "INSERT INTO entries(remote, category, package, desc, type, version, author)"
    "VALUES(?, ?, ?, ?, ?, ?, ?);"
  );

  m_updateEntry = m_db.prepare(
    "UPDATE entries "
    "SET desc = ?, type = ?, version = ?, author = ? WHERE id = ?"
  );

  m_setPinned = m_db.prepare("UPDATE entries SET pinned = ? WHERE id = ?");

  m_findEntry = m_db.prepare(
    "SELECT id, remote, category, package, desc, type, version, author, pinned "
    "FROM entries WHERE remote = ? AND category = ? AND package = ? LIMIT 1"
  );

  m_allEntries = m_db.prepare(
    "SELECT id, remote, category, package, desc, type, version, author, pinned "
    "FROM entries WHERE remote = ?"
  );
  m_forgetEntry = m_db.prepare("DELETE FROM entries WHERE id = ?");

  // file queries
  m_getOwner = m_db.prepare(
    "SELECT e.id, remote, category, package, desc, e.type, version, author, pinned "
    "FROM entries e JOIN files f ON f.entry = e.id WHERE f.path = ? LIMIT 1"
  );
  m_getFiles = m_db.prepare(
    "SELECT path, main, type FROM files WHERE entry = ? ORDER BY path"
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
  const Database::Version version{0, 5};
  const Database::Version &current = m_db.version();

  if(!current) {
    // new database!
    m_db.exec(
      "CREATE TABLE entries ("
      "  id INTEGER PRIMARY KEY,"
      "  remote TEXT NOT NULL,"
      "  category TEXT NOT NULL,"
      "  package TEXT NOT NULL,"
      "  desc TEXT NOT NULL,"
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
      "  type INTEGER NOT NULL,"
      "  FOREIGN KEY(entry) REFERENCES entries(id)"
      ");"
    );

    m_db.setVersion(version);

    return;
  }
  else if(current < version) {
    m_db.begin();

    switch(current.major) {
    case 0:
      // current major schema version
      break;
    default:
      throw reapack_error(
        "The package registry was created by a newer version of ReaPack");
    }

    switch(current.minor) {
    case 1:
      m_db.exec("ALTER TABLE entries ADD COLUMN pinned INTEGER NOT NULL DEFAULT 0;");
      [[fallthrough]];
    case 2:
      m_db.exec("ALTER TABLE files ADD COLUMN type INTEGER NOT NULL DEFAULT 0;");
      [[fallthrough]];
    case 3:
      m_db.exec("ALTER TABLE entries ADD COLUMN desc TEXT NOT NULL DEFAULT '';");
      [[fallthrough]];
    case 4:
      convertImplicitSections();
      break;
    }

    m_db.setVersion(version);
    m_db.commit();
  }
}

auto Registry::push(const Version *ver, vector<Path> *conflicts) -> Entry
{
  bool hasConflicts = false;

  const Package *pkg = ver->package();
  const Category *cat = pkg->category();
  const Index *ri = cat->index();
  const RemotePtr &remote = ri->remote();

  m_db.savepoint();

  m_clearFiles->bind(1, remote->name());
  m_clearFiles->bind(2, cat->name());
  m_clearFiles->bind(3, pkg->name());
  m_clearFiles->exec();

  auto entryId = getEntry(ver->package()).id;

  // register or update package and version
  if(entryId) {
    m_updateEntry->bind(1, pkg->description());
    m_updateEntry->bind(2, pkg->type());
    m_updateEntry->bind(3, ver->name().toString());
    m_updateEntry->bind(4, ver->author());
    m_updateEntry->bind(5, entryId);
    m_updateEntry->exec();
  }
  else {
    m_insertEntry->bind(1, remote->name());
    m_insertEntry->bind(2, cat->name());
    m_insertEntry->bind(3, pkg->name());
    m_insertEntry->bind(4, pkg->description());
    m_insertEntry->bind(5, pkg->type());
    m_insertEntry->bind(6, ver->name().toString());
    m_insertEntry->bind(7, ver->author());
    m_insertEntry->exec();

    entryId = m_db.lastInsertId();
  }

  // register files
  for(const Source *src : ver->sources()) {
    const Path &path = src->targetPath();

    m_insertFile->bind(1, entryId);
    m_insertFile->bind(2, path.join(false));
    m_insertFile->bind(3, src->sections());
    m_insertFile->bind(4, src->typeOverride());

    try {
      m_insertFile->exec();
    }
    catch(const reapack_error &) {
      if(conflicts && m_db.errorCode() == SQLITE_CONSTRAINT) {
        hasConflicts = true;
        conflicts->push_back(path);
      }
      else {
        m_db.restore();
        throw;
      }
    }
  }

  if(hasConflicts) {
    m_db.restore();
    return {};
  }
  else {
    m_db.release();
    return {entryId, remote->name(), cat->name(),
      pkg->name(), pkg->description(), pkg->type(), ver->name(), ver->author()};
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

  m_findEntry->bind(1, ri->remote()->name());
  m_findEntry->bind(2, cat->name());
  m_findEntry->bind(3, pkg->name());

  m_findEntry->exec([&] {
    fillEntry(m_findEntry, &entry);
    return false;
  });

  return entry;
}

auto Registry::getEntries(const string &remoteName) const -> vector<Entry>
{
  vector<Registry::Entry> list;

  m_allEntries->bind(1, remoteName);
  m_allEntries->exec([&] {
    Entry entry{};
    fillEntry(m_allEntries, &entry);
    list.push_back(entry);

    return true;
  });

  return list;
}

auto Registry::getFiles(const Entry &entry) const -> vector<File>
{
  if(!entry) // skip processing for new packages
    return {};

  vector<File> files;

  m_getFiles->bind(1, entry.id);
  m_getFiles->exec([&] {
    int col = 0;

    File file{
      m_getFiles->stringColumn(col++),
      static_cast<int>(m_getFiles->intColumn(col++)),
      static_cast<Package::Type>(m_getFiles->intColumn(col++)),
    };

    if(!file.type) // < v1.0rc2
      file.type = entry.type;

    files.push_back(file);
    return true;
  });

  return files;
}

auto Registry::getMainFiles(const Entry &entry) const -> vector<File>
{
  if(!entry)
    return {};

  const vector<File> &allFiles = getFiles(entry);

  vector<File> mainFiles;
  copy_if(allFiles.begin(), allFiles.end(),
    back_inserter(mainFiles), [&](const File &f) { return f.sections; });

  return mainFiles;
}

auto Registry::getOwner(const Path &path) const -> Entry
{
  Entry entry{};

  m_getOwner->bind(1, path.join(false));

  m_getOwner->exec([&] {
    fillEntry(m_getOwner, &entry);
    return false;
  });

  return entry;
}

void Registry::forget(const Entry &entry)
{
  m_forgetFiles->bind(1, entry.id);
  m_forgetFiles->exec();

  m_forgetEntry->bind(1, entry.id);
  m_forgetEntry->exec();
}

void Registry::convertImplicitSections()
{
  // convert from v1.0 main=true format to v1.1 flag format

  Statement entries("SELECT id, category FROM entries", &m_db);
  entries.exec([&] {
    const auto id = entries.intColumn(0);
    const string &category = entries.stringColumn(1);
    const auto section = Source::detectSection(category);

    Statement update("UPDATE files SET main = ? WHERE entry = ? AND main != 0", &m_db);
    update.bind(1, section);
    update.bind(2, id);
    update.exec();

    return true;
  });
}

void Registry::fillEntry(const Statement *stmt, Entry *entry) const
{
  int col = 0;

  entry->id = stmt->intColumn(col++);
  entry->remote = stmt->stringColumn(col++);
  entry->category = stmt->stringColumn(col++);
  entry->package = stmt->stringColumn(col++);
  entry->description = stmt->stringColumn(col++);
  entry->type = static_cast<Package::Type>(stmt->intColumn(col++));
  entry->version.tryParse(stmt->stringColumn(col++));
  entry->author = stmt->stringColumn(col++);
  entry->pinned = stmt->boolColumn(col++);
}
