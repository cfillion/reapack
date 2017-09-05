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

#include "database.hpp"

#include "errors.hpp"

#include <cinttypes>
#include <sqlite3.h>

Database::Database(const String &fn)
  : m_savePoint(0)
{
  if(sqlite3_open(fn.empty() ? ":memory:" : fn.toUtf8().c_str(), &m_db)) {
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
  const char *error = sqlite3_errmsg(m_db);
  return reapack_error(error);
}

int64_t Database::lastInsertId() const
{
  return sqlite3_last_insert_rowid(m_db);
}

auto Database::version() const -> Version
{
  int32_t version = 0;

  Statement stmt("PRAGMA user_version", this);
  stmt.exec([&] {
    version = static_cast<int32_t>(stmt.intColumn(0));
    return false;
  });

  return {static_cast<int16_t>(version >> 16), static_cast<int16_t>(version)};
}

void Database::setVersion(const Version &version)
{
  int32_t value = version.minor | (int32_t)version.major << 16;

  char sql[255];
  sprintf(sql, "PRAGMA user_version = %" PRId32, value);
  exec(sql);
}

int Database::errorCode() const
{
  return sqlite3_errcode(m_db);
}

void Database::begin()
{
  // IMMEDIATE -> don't wait until the first query to aquire a lock
  // but still allow new read-only connections (unlike EXCLUSIVE)
  // while preventing new transactions to be made as long as it's active
  exec("BEGIN IMMEDIATE TRANSACTION");
}

void Database::commit()
{
  exec("COMMIT");
}

void Database::savepoint()
{
  char sql[64];
  sprintf(sql, "SAVEPOINT sp%zu", m_savePoint++);

  exec(sql);
}

void Database::restore()
{
  char sql[64];
  sprintf(sql, "ROLLBACK TO SAVEPOINT sp%zu", --m_savePoint);

  exec(sql);
}

void Database::release()
{
  char sql[64];
  sprintf(sql, "RELEASE SAVEPOINT sp%zu", --m_savePoint);

  exec(sql);
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

void Statement::bind(const int index, const String &text)
{
  if(sqlite3_bind_text(m_stmt, index, text.toUtf8().c_str(), -1, SQLITE_TRANSIENT))
    throw m_db->lastError();
}

void Statement::bind(const int index, const int64_t integer)
{
  if(sqlite3_bind_int64(m_stmt, index, integer))
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

int64_t Statement::intColumn(const int index) const
{
  return sqlite3_column_int64(m_stmt, index);
}

String Statement::stringColumn(const int index) const
{
  char *col = (char *)sqlite3_column_text(m_stmt, index);

  if(col)
    return col;
  else
    return {};
}
