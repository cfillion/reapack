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

#ifndef REAPACK_CONFIG_HPP
#define REAPACK_CONFIG_HPP

#include <string>

#include "remote.hpp"

class Path;

class Config {
public:
  Config();

  void read(const Path &);
  void write();

  bool isFirstRun() const { return m_isFirstRun; }
  RemoteList *remotes() { return &m_remotes; }

private:
  std::string getString(const char *, const std::string &) const;
  void setString(const char *, const std::string &, const std::string &) const;
  size_t getUInt(const char *, const std::string &) const;
  void setUInt(const char *, const std::string &, const size_t) const;
  void deleteKey(const char *, const std::string &) const;
  void cleanupArray(const char *, const std::string &,
    const size_t begin, const size_t end) const;

  void migrate();

  std::string m_path;
  bool m_isFirstRun;
  size_t m_version;

  void readRemotes();
  void restoreSelfRemote();
  void writeRemotes();
  RemoteList m_remotes;
  size_t m_remotesIniSize;
};

#endif
