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

#include "package.hpp"

#include "errors.hpp"
#include "index.hpp"

#include <algorithm>
#include <boost/range/adaptor/reversed.hpp>

using boost::format;
using namespace std;

Package::Type Package::getType(const char *type)
{
  if(!strcmp(type, "script"))
    return ScriptType;
  else if(!strcmp(type, "extension"))
    return ExtensionType;
  else if(!strcmp(type, "effect"))
    return EffectType;
  else if(!strcmp(type, "data"))
    return DataType;
  else if(!strcmp(type, "theme"))
    return ThemeType;
  else
    return UnknownType;
}

string Package::displayType(const Type type)
{
  switch(type) {
  case ScriptType:
    return "Script";
  case ExtensionType:
    return "Extension";
  case EffectType:
    return "Effect";
  case DataType:
    return "Data";
  case ThemeType:
    return "Theme";
  default:
    return "Unknown";
  }
}

const string &Package::displayName(const string &name,
  const string &desc, const bool enableDesc)
{
  if(!enableDesc || desc.empty())
    return name;
  else
    return desc;
}

Package::Package(const Type type, const string &name, const Category *cat)
  : m_category(cat), m_type(type), m_name(name)
{
  if(m_name.empty())
    throw reapack_error("empty package name");
  else if(m_name.find_first_of("/\\") != string::npos)
    throw reapack_error(format("invalid package name '%s'") % m_name);
}

Package::~Package()
{
  for(const Version *ver : m_versions)
    delete ver;
}

string Package::fullName() const
{
  return m_category ? m_category->fullName() + "/" + displayName() : displayName();
}

bool Package::addVersion(const Version *ver)
{
  if(ver->package() != this)
    throw reapack_error("version belongs to another package");
  else if(ver->sources().empty())
    return false;
  else if(m_versions.count(ver))
    throw reapack_error(format("duplicate version '%s'") % ver->fullName());

  m_versions.insert(ver);

  return true;
}

const Version *Package::version(const size_t index) const
{
  auto it = m_versions.begin();

  for(size_t i = 0; i < index; i++)
    it++;

  return *it;
}

const Version *Package::lastVersion(const bool pres, const Version &from) const
{
  if(m_versions.empty())
    return nullptr;

  for(const Version *ver : m_versions | boost::adaptors::reversed) {
    if(*ver < from)
      break;
    else if(ver->isStable() || pres)
      return ver;
  }

  return from.isStable() ? nullptr : *m_versions.rbegin();
}

const Version *Package::findVersion(const Version &ver) const
{
  const auto &it = find_if(m_versions.begin(), m_versions.end(),
    [=] (const Version *cur) { return *cur == ver; });

  if(it == m_versions.end())
    return nullptr;
  else
    return *it;
}
