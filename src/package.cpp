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
#include <sstream>

using namespace std;

Package::Type Package::typeFor(const char *type)
{
  if(!strcmp(type, "script"))
    return ScriptType;
  else if(!strcmp(type, "extension"))
    return ExtensionType;
  else if(!strcmp(type, "effect"))
    return EffectType;
  else if(!strcmp(type, "data"))
    return DataType;
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
  default:
    return "Unknown";
  }
}

Package::Package(const Type type, const string &name, const Category *cat)
  : m_category(cat), m_type(type), m_name(name)
{
  if(m_name.empty())
    throw reapack_error("empty package name");
}

Package::~Package()
{
  for(const Version *ver : m_versions)
    delete ver;
}

string Package::fullName() const
{
  return m_category ? m_category->fullName() + "/" + m_name : m_name;
}

string Package::displayType() const
{
  return displayType(m_type);
}

void Package::addVersion(const Version *ver)
{
  if(ver->package() != this)
    throw reapack_error("version belongs to another package");

  if(ver->sources().empty())
    return;

  m_versions.insert(ver);
}

const Version *Package::version(const size_t index) const
{
  auto it = m_versions.begin();

  for(size_t i = 0; i < index; i++)
    it++;

  return *it;
}

const Version *Package::lastVersion() const
{
  if(m_versions.empty())
    return nullptr;

  return *m_versions.rbegin();
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

Path Package::makeTargetPath(const string &file) const
{
  Path path;

  if(!m_category || !m_category->index())
    throw reapack_error("category or index is unset");

  // select the target directory
  switch(m_type) {
  case ScriptType:
    path.append("Scripts");
    break;
  case EffectType:
    path.append("Effects");
    break;
  case DataType:
    path.append("Data");
    break;
  case ExtensionType:
    path.append("UserPlugins");
    break;
  case UnknownType:
    // The package has an unsupported type, so we return an empty path.
    // The empty path won't be used because the category will reject
    // this package right away. Maybe the parser should not bother with loading
    // unsupported packages at all anyway... But then in the future
    // we might want to display unsupported packages in the interface.
    break;
  }

  // append the rest of the path
  switch(m_type) {
  case ScriptType:
  case EffectType:
  case DataType:
    path.append(m_category->index()->name());

    // only allow directory traversal up to the index name
    path += Path(m_category->name()) + file;
    break;
  case ExtensionType:
    path.append(file, false);
    break;
  case UnknownType:
    break;
  }

  return path;
}
