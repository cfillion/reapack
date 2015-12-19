/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015  Christian Fillion
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

#include <string>
#include <vector>

#include "path.hpp"

class Source;
typedef std::vector<Source *> SourceList;

class Package;
class Version;

class Source {
public:
  enum Platform {
    UnknownPlatform,
    GenericPlatform,

    // windows
    WindowsPlatform,
    Win32Platform,
    Win64Platform,

    // os x
    DarwinPlatform,
    Darwin32Platform,
    Darwin64Platform,
  };

  static Platform ConvertPlatform(const char *);

  Source(const Platform, const std::string &file,
    const std::string &url, Version * = nullptr);

  Platform platform() const { return m_platform; }
  const std::string &file() const;
  const std::string &url() const { return m_url; }

  Version *version() const { return m_version; }
  Package *package() const;

  std::string fullName() const;
  Path targetPath() const;

private:
  Platform m_platform;
  std::string m_file;
  std::string m_url;
  Version *m_version;
};

#endif
