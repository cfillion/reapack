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

#ifndef REAPACK_CONFIG_HPP
#define REAPACK_CONFIG_HPP

#include "string.hpp"
#include "remote.hpp"

class Path;

struct WindowState {
  String about;
  String browser;
  String manager;
};

struct InstallOpts {
  bool autoInstall;
  bool bleedingEdge;
  bool promptObsolete;
};

struct NetworkOpts {
  enum StaleThreshold {
    NoThreshold = 0,
    OneWeekThreshold = 7 * 24 * 3600,
  };

  String proxy;
  bool verifyPeer;
  time_t staleThreshold;
};

class Config {
public:
  Config();

  void read(const Path &);
  void write();

  void resetOptions();
  void restoreDefaultRemotes();

  bool isFirstRun() const { return m_isFirstRun; }

  InstallOpts install;
  NetworkOpts network;
  WindowState windowState;

  RemoteList remotes;

private:
  String getString(const Char *grp,
    const String &key, const String &fallback = {}) const;
  void setString(const Char *grp,
    const String &key, const String &val) const;

  bool getBool(const Char *grp,
    const String &key, bool fallback = false) const;
  unsigned int getUInt(const Char *grp,
    const String &key, unsigned int fallback = 0) const;
  void setUInt(const Char *, const String &, unsigned int) const;

  void deleteKey(const Char *, const String &) const;
  void cleanupArray(const Char *, const String &,
    unsigned int begin, unsigned int end) const;

  void migrate();

  String m_path;
  bool m_isFirstRun;
  unsigned int m_version;

  void readRemotes();
  void restoreSelfRemote();
  void writeRemotes();
  unsigned int m_remotesIniSize;
};

#endif
