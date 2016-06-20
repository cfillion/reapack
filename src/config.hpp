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

#include "encoding.hpp"
#include "remote.hpp"

class Path;

struct InstallOpts {
  bool autoInstall;
  bool bleedingEdge;
};

struct BrowserOpts {
  unsigned int typeFilter;
  bool showDescs;
  std::string list;
};

struct NetworkOpts {
  std::string proxy;
  bool verifyPeer;
};

class Config {
public:
  Config();

  void read(const Path &);
  void write();

  void resetOptions();
  void restoreDefaultRemotes();

  bool isFirstRun() const { return m_isFirstRun; }
  InstallOpts *install() { return &m_install; }
  BrowserOpts *browser() { return &m_browser; }
  NetworkOpts *network() { return &m_network; }
  RemoteList *remotes() { return &m_remotes; }

private:
  std::string getString(const auto_char *grp,
    const auto_string &key, const std::string &fallback = {}) const;
  void setString(const auto_char *grp,
    const auto_string &key, const std::string &val) const;

  unsigned int getUInt(const auto_char *grp,
    const auto_string &key, unsigned int fallback = 0) const;
  void setUInt(const auto_char *, const auto_string &, unsigned int) const;

  void deleteKey(const auto_char *, const auto_string &) const;
  void cleanupArray(const auto_char *, const auto_string &,
    unsigned int begin, unsigned int end) const;

  void migrate();

  auto_string m_path;
  bool m_isFirstRun;
  unsigned int m_version;

  InstallOpts m_install;
  BrowserOpts m_browser;
  NetworkOpts m_network;

  void readRemotes();
  void restoreSelfRemote();
  void writeRemotes();
  RemoteList m_remotes;
  unsigned int m_remotesIniSize;
};

#endif
