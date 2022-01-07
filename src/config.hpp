/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2022  Christian Fillion
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

#include "remote.hpp"

#include <string>

class Path;

struct WindowState {
  std::string about;
  std::string browser;
  std::string manager;
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

  std::string proxy;
  bool verifyPeer;
  time_t staleThreshold;
};

struct FilterOpts {
  bool expandSynonyms;
};

class Config {
public:
  Config(const Path &);
  Config(const Config &) = delete;
  ~Config();

  void read();
  void write();

  void resetOptions();
  void restoreDefaultRemotes();

  bool isFirstRun() const { return m_isFirstRun; }

  InstallOpts install;
  NetworkOpts network;
  FilterOpts  filter;
  WindowState windowState;

  RemoteList remotes;

private:
  std::string getString(const char *g, const char *k, const std::string &fallback = {}) const;
  void setString(const char *g, const char *k, const std::string &v) const;

  bool getBool(const char *g, const char *k, bool fallback = false) const;
  unsigned int getUInt(const char *g, const char *k, unsigned int fallback = 0) const;
  void setUInt(const char *g, const char *k, unsigned int v) const;

  void deleteKey(const char *g, const char *k) const;
  void cleanupArray(const char *g, const char *k, unsigned int begin, unsigned int end) const;

  void migrate();

  std::string m_path;
  bool m_isFirstRun;
  unsigned int m_version;

  void readRemotes();
  void restoreSelfRemote();
  void writeRemotes();
  unsigned int m_remotesIniSize;
};

#endif
