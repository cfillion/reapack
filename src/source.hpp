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

#ifndef REAPACK_SOURCE_HPP
#define REAPACK_SOURCE_HPP

#include "package.hpp"
#include "platform.hpp"

class Package;
class Path;
class Version;

class Source {
public:
  Source(const std::string &file, const std::string &url,
    const Version * = nullptr);

  void setPlatform(Platform p) { m_platform = p; }
  Platform platform() const { return m_platform; }

  void setTypeOverride(Package::Type t) { m_type = t; }
  Package::Type typeOverride() const { return m_type; }
  Package::Type type() const;
  const std::string &file() const;
  const std::string &url() const { return m_url; }
  void setMain(bool main) { m_main = main; }
  bool isMain() const { return m_main; }

  const Version *version() const { return m_version; }
  const Package *package() const;

  std::string fullName() const;
  Path targetPath() const;

private:
  Platform m_platform;
  Package::Type m_type;
  std::string m_file;
  std::string m_url;
  bool m_main;
  const Version *m_version;
};

#endif
