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

#ifndef REAPACK_SOURCE_HPP
#define REAPACK_SOURCE_HPP

#include "package.hpp"
#include "path.hpp"
#include "platform.hpp"

class Package;
class Version;

class Source {
public:
  enum Section {
    UnknownSection          = 0,
    MainSection             = 1<<0,
    MIDIEditorSection       = 1<<1,
    MIDIInlineEditorSection = 1<<2,

    ImplicitSection = -1, // for compatibility with v1.0
  };

  static Section getSection(const char *);
  static Section detectSection(const std::string &category);

  Source(const std::string &file, const std::string &url, const Version *);
  const Version *version() const { return m_version; }

  void setPlatform(Platform p) { m_platform = p; }
  Platform platform() const { return m_platform; }
  void setTypeOverride(Package::Type t) { m_type = t; }
  Package::Type typeOverride() const { return m_type; }
  Package::Type type() const;
  const std::string &file() const;
  const std::string &url() const { return m_url; }
  void setSections(int);
  int sections() const { return m_sections; }

  Path targetPath() const;

private:
  Platform m_platform;
  Package::Type m_type;
  std::string m_file;
  std::string m_url;
  int m_sections;
  Path m_targetPath;
  const Version *m_version;
};

#endif
