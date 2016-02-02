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
  else
    return UnknownType;
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

Package::~Package()
{
  for(Version *ver : m_versions)
    delete ver;
}

void Package::addVersion(Version *ver)
{
  if(ver->package() != this)
    throw reapack_error("version belongs to another package");

  if(ver->sources().empty())
    return;

  m_versions.insert(ver);
}

Version *Package::version(const size_t index) const
{
  auto it = m_versions.begin();

  for(size_t i = 0; i < index; i++)
    it++;

  return *it;
}

Version *Package::lastVersion() const
{
  if(m_versions.empty())
    return nullptr;

  return *m_versions.rbegin();
}

Path Package::targetPath() const
{
  Path path;

  if(!m_category || !m_category->index())
    throw reapack_error("category or index is unset");

  switch(m_type) {
  case ScriptType:
    path.append("Scripts");
    path.append(m_category->index()->name());
    path.append(m_category->name());
    break;
  default:
    throw reapack_error("unsupported package type");
  }

  return path;
}
