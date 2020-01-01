/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2020  Christian Fillion
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

#ifndef REAPACK_REMOTE_HPP
#define REAPACK_REMOTE_HPP

#include <boost/logic/tribool.hpp>
#include <map>
#include <string>
#include <vector>

typedef boost::logic::tribool tribool;

class Remote {
public:
  static Remote fromString(const std::string &data);

  Remote();
  Remote(const std::string &name, const std::string &url, bool enabled = true,
    const tribool &autoInstall = boost::logic::indeterminate);

  std::string toString() const;

  void setName(const std::string &name);
  const std::string &name() const { return m_name; }

  void setUrl(const std::string &url);
  const std::string &url() const { return m_url; }

  bool isNull() const { return m_name.empty() || m_url.empty(); }

  void enable() { setEnabled(true); }
  void disable() { setEnabled(false); }
  void setEnabled(const bool enabled) { m_enabled = enabled; }
  bool isEnabled() const { return m_enabled; }

  void setAutoInstall(const tribool &autoInstall) { m_autoInstall = autoInstall; }
  tribool autoInstall() const { return m_autoInstall; }
  bool autoInstall(bool fallback) const;

  void protect() { m_protected = true; }
  bool isProtected() const { return m_protected; }

  bool operator<(const Remote &o) const { return m_name < o.name(); }
  operator bool() const { return !isNull(); }

private:
  std::string m_name;
  std::string m_url;
  bool m_enabled;
  bool m_protected;
  tribool m_autoInstall;
};

class RemoteList {
public:
  RemoteList() {}
  RemoteList(const RemoteList &) = delete;

  void add(const Remote &);
  void remove(const Remote &remote) { remove(remote.name()); }
  void remove(const std::string &name);
  Remote get(const std::string &name) const;
  std::vector<Remote> getEnabled() const;

  bool empty() const { return m_remotes.empty(); }
  size_t size() const { return m_remotes.size(); }
  bool hasName(const std::string &name) const { return m_map.count(name) > 0; }

  std::vector<Remote>::const_iterator begin() const { return m_remotes.begin(); }
  std::vector<Remote>::const_iterator end() const { return m_remotes.end(); }

private:
  std::vector<Remote> m_remotes;
  std::map<std::string, size_t> m_map;
};

#endif
