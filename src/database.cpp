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

#include "database.hpp"

#include "errors.hpp"

#include <sqlite3.h>

using namespace std;

Database::Database(const string &filename)
{
  const char *file = ":memory:";

  if(!filename.empty())
    file = filename.c_str();

  if(sqlite3_open(file, &m_db)) {
    const auto &error = lastError();
    sqlite3_close(m_db);

    throw error;
  }

  exec("PRAGMA foreign_keys = 1");
}

Database::~Database()
{
  for(Statement *stmt : m_statements)
    delete stmt;

  sqlite3_close(m_db);
}

Statement *Database::prepare(const char *sql)
{
  Statement *stmt = new Statement(sql, this);
  m_statements.push_back(stmt);

  return stmt;
}

void Database::exec(const char *sql)
{
  if(sqlite3_exec(m_db, sql, nullptr, nullptr, nullptr) != SQLITE_OK)
    throw lastError();
}

reapack_error Database::lastError() const
{
  return reapack_error(sqlite3_errmsg(m_db));
}

uint64_t Database::lastInsertId() const
{
  return (uint64_t)sqlite3_last_insert_rowid(m_db);
}

int Database::version() const
{
  int version = 0;

  Statement stmt("PRAGMA user_version", this);
  stmt.exec([&] {
    version = stmt.intColumn(0);
    return false;
  });

  return version;
}

int Database::errorCode() const
{
  return sqlite3_extended_errcode(m_db);
}

void Database::begin()
{
  // EXCLUSIVE -> don't wait until the first query to aquire a lock
  exec("BEGIN EXCLUSIVE TRANSACTION");
}

void Database::commit()
{
  exec("COMMIT");
}

Statement::Statement(const char *sql, const Database *db)
  : m_db(db)
{
  if(sqlite3_prepare_v2(db->m_db, sql, -1, &m_stmt, nullptr) != SQLITE_OK)
    throw m_db->lastError();
}

Statement::~Statement()
{
  sqlite3_finalize(m_stmt);
}

void Statement::bind(const int index, const string &text)
{
  if(sqlite3_bind_text(m_stmt, index, text.c_str(), -1, SQLITE_TRANSIENT))
    throw m_db->lastError();
}

void Statement::bind(const int index, const int integer)
{
  if(sqlite3_bind_int(m_stmt, index, integer))
    throw m_db->lastError();
}

void Statement::bind(const int index, const uint64_t integer)
{
  if(sqlite3_bind_int64(m_stmt, index, (sqlite3_int64)integer))
    throw m_db->lastError();
}

void Statement::exec()
{
  exec([=] { return false; });
}

void Statement::exec(const ExecCallback &callback)
{
  while(true) {
    switch(sqlite3_step(m_stmt)) {
    case SQLITE_ROW:
      if(callback())
        break;
    case SQLITE_DONE:
      sqlite3_clear_bindings(m_stmt);
      sqlite3_reset(m_stmt);
      return;
    default:
      sqlite3_clear_bindings(m_stmt);
      sqlite3_reset(m_stmt);
      throw m_db->lastError();
    };
  }
}

int Statement::intColumn(const int index) const
{
  return sqlite3_column_int(m_stmt, index);
}

uint64_t Statement::uint64Column(const int index) const
{
  return (uint64_t)sqlite3_column_int64(m_stmt, index);
}

string Statement::stringColumn(const int index) const
{
  return (char *)sqlite3_column_text(m_stmt, index);
}
