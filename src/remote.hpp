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

#ifndef REAPACK_REMOTE_HPP
#define REAPACK_REMOTE_HPP

#include <boost/optional.hpp>
#include <boost/range/adaptor/map.hpp>
#include <map>
#include <string>

class Remote {
public:
  enum ReadCode {
    Success,
    ReadFailure,
    InvalidName,
    InvalidUrl,
  };

  static ReadCode fromFile(const std::string &path, Remote *remote);

  Remote();
  Remote(const std::string &name, const std::string &url,
    const bool enabled = true);

  void setName(const std::string &name);
  const std::string &name() const { return m_name; }

  void setUrl(const std::string &url);
  const std::string &url() const { return m_url; }

  bool isNull() const { return m_name.empty() || m_url.empty(); }
  bool isValid() const { return !isNull(); }

  void enable() { setEnabled(true); }
  void disable() { setEnabled(false); }
  void setEnabled(const bool enabled) { m_enabled = enabled; }
  bool isEnabled() const { return m_enabled; }

  void freeze() { m_frozen = true; }
  bool isFrozen() const { return m_frozen; }

private:
  std::string m_name;
  std::string m_url;
  bool m_enabled;
  bool m_frozen;
};

class RemoteList {
private:
  std::map<std::string, Remote> m_remotes;

public:
  void add(const Remote &);

  bool empty() const { return m_remotes.empty(); }
  size_t size() const { return m_remotes.size(); }
  bool hasName(const std::string &name) const
  { return m_remotes.count(name) > 0; }
  bool hasUrl(const std::string &url) const;

  auto begin() const -> decltype(boost::adaptors::values(m_remotes).begin())
  { return boost::adaptors::values(m_remotes).begin(); }

  auto end() const -> decltype(boost::adaptors::values(m_remotes).end())
  { return boost::adaptors::values(m_remotes).end(); }

  Remote get(const std::string &name) const;
};

#endif
