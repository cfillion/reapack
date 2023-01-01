/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2023  Christian Fillion
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

Package::Type Package::getType(const char *type)
{
  constexpr std::pair<const char *, Type> map[] {
    {"script",        ScriptType},
    {"extension",     ExtensionType},
    {"effect",        EffectType},
    {"data",          DataType},
    {"theme",         ThemeType},
    {"langpack",      LangPackType},
    {"webinterface",  WebInterfaceType},
    {"projecttpl",    ProjectTemplateType},
    {"tracktpl",      TrackTemplateType},
    {"midinotenames", MIDINoteNamesType},
    {"autoitem",      AutomationItemType},
  };

  for(auto &[key, value] : map) {
    if(!strcmp(type, key))
      return value;
  }

  return UnknownType;
}

const char *Package::displayType(const Type type)
{
  switch(type) {
  case UnknownType:
    break;
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
  case LangPackType:
    return "Language Pack";
  case WebInterfaceType:
    return "Web Interface";
  case ProjectTemplateType:
    return "Project Template";
  case TrackTemplateType:
    return "Track Template";
  case MIDINoteNamesType:
    return "MIDI Note Names";
  case AutomationItemType:
    return "Automation Item";
  }

  return "Unknown";
}

const std::string &Package::displayName(const std::string &name, const std::string &desc)
{
  return desc.empty() ? name : desc;
}

Package::Package(const Type type, const std::string &name, const Category *cat)
  : m_category(cat), m_type(type), m_name(name)
{
  if(m_name.empty())
    throw reapack_error("empty package name");
  else if(m_name.find_first_of("/\\") != std::string::npos) {
    throw reapack_error(
      String::format("invalid package name '%s'", m_name.c_str()));
  }
}

Package::~Package()
{
  for(const Version *ver : m_versions)
    delete ver;
}

std::string Package::fullName() const
{
  return m_category ? m_category->fullName() + "/" + displayName() : displayName();
}

bool Package::addVersion(const Version *ver)
{
  if(ver->package() != this)
    throw reapack_error("version belongs to another package");
  else if(ver->sources().empty())
    return false;
  else if(m_versions.count(ver)) {
    throw reapack_error(String::format("duplicate version '%s'",
      ver->fullName().c_str()));
  }

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
