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
  else
    return UnknownType;
}

string Package::displayType(const Type type)
{
  switch(type) {
  case UnknownType:
    return "Unknown";
  case ScriptType:
    return "Script";
  case ExtensionType:
    return "Extension";
  case EffectType:
    return "Effect";
  }

  return {}; // MSVC is stupid
}

Package::Package(const Type type, const string &name, Category *cat)
  : m_category(cat), m_type(type), m_name(name)
{
  if(m_name.empty())
    throw reapack_error("empty package name");
}

string Package::fullName() const
{
  return m_category ? m_category->fullName() + "/" + m_name : m_name;
}

string Package::displayType() const
{
  return displayType(m_type);
}

Package::~Package()
{
  for(const Version *ver : m_versions)
    delete ver;
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

Path Package::makeTargetPath(const string &file) const
{
  Path path;

  if(!m_category || !m_category->index())
    throw reapack_error("category or index is unset");

  switch(m_type) {
  case ScriptType:
    path.append("Scripts");
    path.append(m_category->index()->name());

    // only allow directory traversal up to the index name
    path += Path(m_category->name()) + file;
    break;
  case EffectType:
    path.append("Effects");
    path.append(m_category->index()->name());
    path += Path(m_category->name()) + file;
    break;
  case ExtensionType:
    path.append("UserPlugins");
    path.append(file, false);
    break;
  case UnknownType:
    // The package has an unsupported type, so we return an empty path.
    // The empty path won't be used because the category will reject
    // this package right away. Maybe the parser should not bother with loading
    // unsupported packages at all anyway... But then in the future
    // we might want to display unsupported packages in the interface.
    break;
  }

  return path;
}
