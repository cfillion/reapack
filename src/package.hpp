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

#ifndef REAPACK_PACKAGE_HPP
#define REAPACK_PACKAGE_HPP

#include "path.hpp"
#include "version.hpp"

class Category;

class Package;
typedef std::vector<const Package *> PackageList;

class Package {
public:
  enum Type {
    UnknownType,
    ScriptType,
    ExtensionType,
    EffectType,
  };

  static Type typeFor(const char *);

  Package(const Type, const std::string &name, Category * = nullptr);
  ~Package();

  Category *category() const { return m_category; }
  Type type() const { return m_type; }
  std::string displayType() const;
  const std::string &name() const { return m_name; }
  std::string fullName() const;

  void addVersion(const Version *ver);
  const VersionSet &versions() const { return m_versions; }
  const Version *version(size_t index) const;
  const Version *lastVersion() const;

  Path makeTargetPath(const std::string &file = {}) const;

private:
  Category *m_category;

  Type m_type;
  std::string m_name;
  VersionSet m_versions;
};

#endif
