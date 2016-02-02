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
typedef std::vector<Package *> PackageList;

class Package {
public:
  enum Type {
    UnknownType,
    ScriptType,
  };

  static Type ConvertType(const char *);

  Package(const Type, const std::string &name, Category * = nullptr);
  ~Package();

  Category *category() const { return m_category; }
  Type type() const { return m_type; }
  const std::string &name() const { return m_name; }
  std::string fullName() const;

  void setAuthor(const std::string &author) { m_author = author; }
  const std::string &author() const { return m_author; }

  void addVersion(Version *ver);
  const VersionSet &versions() const { return m_versions; }
  Version *version(const size_t i) const;
  Version *lastVersion() const;

  Path targetPath() const;

private:
  Category *m_category;

  Type m_type;
  std::string m_name;
  std::string m_author;
  VersionSet m_versions;
};

#endif
