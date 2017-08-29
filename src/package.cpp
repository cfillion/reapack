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

#include "package.hpp"

#include "errors.hpp"
#include "index.hpp"

#include <algorithm>
#include <boost/range/adaptor/reversed.hpp>

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
  else if(!strcmp(type, "langpack"))
    return LangPackType;
  else if(!strcmp(type, "webinterface"))
    return WebInterfaceType;
  else if(!strcmp(type, "projecttpl"))
    return ProjectTemplateType;
  else if(!strcmp(type, "tracktpl"))
    return TrackTemplateType;
  else if(!strcmp(type, "midinotenames"))
    return MIDINoteNamesType;
  else
    return UnknownType;
}

String Package::displayType(const Type type)
{
  switch(type) {
  case UnknownType:
    break;
  case ScriptType:
    return L"Script";
  case ExtensionType:
    return L"Extension";
  case EffectType:
    return L"Effect";
  case DataType:
    return L"Data";
  case ThemeType:
    return L"Theme";
  case LangPackType:
    return L"Language Pack";
  case WebInterfaceType:
    return L"Web Interface";
  case ProjectTemplateType:
    return L"Project Template";
  case TrackTemplateType:
    return L"Track Template";
  case MIDINoteNamesType:
    return L"MIDI Note Names";
  }

  return "Unknown";
}

const String &Package::displayName(const String &name, const String &desc)
{
  return desc.empty() ? name : desc;
}

Package::Package(const Type type, const String &name, const Category *cat)
  : m_category(cat), m_type(type), m_name(name)
{
  if(m_name.empty())
    throw reapack_error(L"empty package name");
  else if(m_name.find_first_of(L"/\\") != string::npos)
    throw reapack_error(StringFormat(L"invalid package name '%s'") % m_name);
}

Package::~Package()
{
  for(const Version *ver : m_versions)
    delete ver;
}

String Package::fullName() const
{
  if(!m_category)
    return displayName();

  String out = m_category->fullName();
  out += L'/';
  out += displayName();
  return out;
}

bool Package::addVersion(const Version *ver)
{
  if(ver->package() != this)
    throw reapack_error(L"version belongs to another package");
  else if(ver->sources().empty())
    return false;
  else if(m_versions.count(ver))
    throw reapack_error(StringFormat(L"duplicate version '%s'") % ver->fullName());

  m_versions.insert(ver);

  return true;
}

const Version *Package::version(const size_t index) const
{
  auto it = m_versions.begin();
  advance(it, index);

  return *it;
}

const Version *Package::lastVersion(const bool pres, const VersionName &from) const
{
  if(m_versions.empty())
    return nullptr;

  for(const Version *ver : m_versions | boost::adaptors::reversed) {
    if(ver->name() < from)
      break;
    else if(ver->name().isStable() || pres)
      return ver;
  }

  return from.isStable() ? nullptr : *m_versions.rbegin();
}

const Version *Package::findVersion(const VersionName &ver) const
{
  const auto &it = find_if(m_versions.begin(), m_versions.end(),
    [=] (const Version *cur) { return cur->name() == ver; });

  if(it == m_versions.end())
    return nullptr;
  else
    return *it;
}
