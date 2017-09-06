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

#ifndef REAPACK_PACKAGE_HPP
#define REAPACK_PACKAGE_HPP

#include "metadata.hpp"
#include "version.hpp"

class Category;

class Package {
public:
  enum Type {
    UnknownType,
    ScriptType,
    ExtensionType,
    EffectType,
    DataType,
    ThemeType,
    LangPackType,
    WebInterfaceType,
    ProjectTemplateType,
    TrackTemplateType,
    MIDINoteNamesType,
  };

  static Type getType(const char *);
  static std::string displayType(Type);
  static const std::string &displayName(const std::string &name, const std::string &desc);

  Package(const Type, const std::string &name, const Category *);
  ~Package();

  const Category *category() const { return m_category; }
  Type type() const { return m_type; }
  std::string displayType() const { return displayType(m_type); }
  const std::string &name() const { return m_name; }
  std::string fullName() const;
  void setDescription(const std::string &d) { m_desc = d; }
  const std::string &description() const { return m_desc; }
  const std::string &displayName() const
    { return displayName(m_name, m_desc); }

  Metadata *metadata() { return &m_metadata; }
  const Metadata *metadata() const { return &m_metadata; }

  bool addVersion(const Version *ver);
  const auto &versions() const { return m_versions; }
  const Version *version(size_t index) const;
  const Version *lastVersion(bool pres = true, const VersionName &from = {}) const;
  const Version *findVersion(const VersionName &) const;

private:
  class CompareVersion {
  public:
    bool operator()(const Version *l, const Version *r) const
    {
      return l->name() < r->name();
    }
  };

  const Category *m_category;

  Type m_type;
  std::string m_name;
  std::string m_desc;
  Metadata m_metadata;
  std::set<const Version *, CompareVersion> m_versions;

};

#endif
