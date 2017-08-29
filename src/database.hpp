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

#ifndef REAPACK_DATABASE_HPP
#define REAPACK_DATABASE_HPP

#include "string.hpp"

#include <cstdint>
#include <functional>
#include <vector>

class reapack_error;

struct sqlite3;
struct sqlite3_stmt;

class Statement;

class Database {
public:
  struct Version {
    int16_t major;
    int16_t minor;

    operator bool() const { return major || minor; }

    bool operator<(const Version &o) const
    {
      return major == o.major ? minor < o.minor : major < o.major;
    }
  };

  Database(const String &filename = {});
  ~Database();

  Statement *prepare(const char *sql);
  void exec(const char *sql);
  int64_t lastInsertId() const;
  Version version() const;
  void setVersion(const Version &);
  int errorCode() const;

  void begin();
  void commit();
  void savepoint();
  void restore();
  void release();

private:
  friend Statement;

  reapack_error lastError() const;

  sqlite3 *m_db;
  std::vector<Statement *> m_statements;
  size_t m_savePoint;
};

class Statement {
public:
  typedef std::function<bool (void)> ExecCallback;

  Statement(const char *sql, const Database *db);
  ~Statement();

  void bind(int index, const String &text);
  void bind(int index, int64_t integer);
  void exec();
  void exec(const ExecCallback &);

  int64_t intColumn(int index) const;
  bool boolColumn(int index) const { return intColumn(index) != 0; }
  String stringColumn(int index) const;

private:
  friend Database;

  const Database *m_db;
  sqlite3_stmt *m_stmt;
};

#endif
