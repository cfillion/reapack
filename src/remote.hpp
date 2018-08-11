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

#ifndef REAPACK_REMOTE_HPP
#define REAPACK_REMOTE_HPP

#include <boost/logic/tribool.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <functional>
#include <memory>
#include <string>
#include <vector>

class Index;
typedef std::shared_ptr<const Index> IndexPtr;

class Remote;
typedef std::shared_ptr<Remote> RemotePtr;

class Remote : public std::enable_shared_from_this<Remote> {
public:
  enum Flag {
    ProtectedFlag = 1<<0,
  };

  static RemotePtr fromString(const std::string &data);

  Remote(const std::string &name);
  Remote(const std::string &name, const std::string &url);

  std::string toString() const;

  const std::string &name() const { return m_name; }
  const std::string &url() const { return m_url; }
  bool isEnabled() const { return m_enabled; }
  boost::tribool autoInstall() const { return m_autoInstall; }
  bool autoInstall(bool fallback) const;

  void setUrl(const std::string &url);
  void setEnabled(const bool enabled);
  void setAutoInstall(const boost::tribool &autoInstall);

  bool test(Flag f) const { return (m_flags & f) != 0; }
  int flags() const { return m_flags; }
  void protect() { m_flags |= ProtectedFlag; }

  void about(bool focus = true);

  IndexPtr loadIndex();
  bool fetchIndex(const std::function<void (const IndexPtr &)> &);
  IndexPtr index() const { return m_index.lock(); }

private:
  friend void swap(Remote &a, Remote &b)
  {
    std::swap(a.m_name, b.m_name);
    std::swap(a.m_url, b.m_url);
    std::swap(a.m_enabled, b.m_enabled);
    std::swap(a.m_autoInstall, b.m_autoInstall);
    std::swap(a.m_flags, b.m_flags);
    std::swap(a.m_index, b.m_index);
  }

  void setName(const std::string &name);

  std::string m_name;
  std::string m_url;
  bool m_enabled;
  boost::tribool m_autoInstall;
  int m_flags;
  std::weak_ptr<const Index> m_index;
};

class RemoteList {
public:
  RemoteList() {}
  RemoteList(const RemoteList &) = delete;

  void add(const RemotePtr &);
  void remove(const RemotePtr &);

  RemotePtr getByName(const std::string &name) const;
  RemotePtr getSelf() const { return getByName("ReaPack"); }

  bool empty() const { return m_remotes.empty(); }
  size_t size() const { return m_remotes.size(); }

  auto begin() { return m_remotes.begin(); }
  auto begin() const { return m_remotes.begin(); }
  auto end() { return m_remotes.end(); }
  auto end() const { return m_remotes.end(); }

  auto enabled() const
  {
    const auto filter = std::bind(&Remote::isEnabled, std::placeholders::_1);
    return m_remotes | boost::adaptors::filtered(filter);
  }

private:
  std::vector<RemotePtr> m_remotes;
};

#endif
