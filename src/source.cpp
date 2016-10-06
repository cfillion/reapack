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

#include "source.hpp"

#include "errors.hpp"
#include "index.hpp"

#include <boost/algorithm/string.hpp>

using namespace std;

auto Source::getSection(const char *name) -> Section
{
  if(!strcmp(name, "main"))
    return MainSection;
  else if(!strcmp(name, "midi_editor"))
    return MIDIEditorSection;
  else if(!strcmp(name, "true"))
    return ImplicitSection;
  else
    return UnknownSection;
}

auto Source::detectSection(const string &category) -> Section
{
  // this is for compatibility with v1.0

  string topcategory = Path(category).first();
  boost::algorithm::to_lower(topcategory);

  if(topcategory == "midi editor")
    return MIDIEditorSection;
  else
    return MainSection;
}

Source::Source(const string &file, const string &url, const Version *ver)
  : m_type(Package::UnknownType), m_file(file), m_url(url), m_sections(0),
    m_version(ver)
{
  if(m_url.empty())
    throw reapack_error("empty source url");
}

const Package *Source::package() const
{
  return m_version ? m_version->package() : nullptr;
}

Package::Type Source::type() const
{
  if(m_type)
    return m_type;
  else if(const Package *pkg = package())
    return pkg->type();
  else
    return Package::UnknownType;
}

const string &Source::file() const
{
  if(!m_file.empty())
    return m_file;

  if(const Package *pkg = package())
    return pkg->name();
  else
    throw reapack_error("empty source file name and no package");
}

void Source::setSections(int sections)
{
  if(sections == ImplicitSection) {
    const Package *pkg = package();
    const Category *cat = pkg ? pkg->category() : nullptr;
    if(!cat)
      throw reapack_error("cannot resolve implicit section: category is unset");

    sections = detectSection(cat->name());
  }

  m_sections = sections;
}

string Source::fullName() const
{
  if(!m_version)
    return Path(file()).basename();
  else if(m_file.empty())
    return m_version->fullName();

  return m_version->fullName() + " (" + Path(m_file).basename() + ")";
}

Path Source::targetPath() const
{
  const Package *pkg = package();
  if(!pkg)
    throw reapack_error("no package associated with this source");

  const Category *cat = pkg->category();
  if(!cat || !cat->index())
    throw reapack_error("category or index is unset");

  Path path;
  const auto type = this->type();

  // select the target directory
  switch(type) {
  case Package::ScriptType:
    path.append("Scripts");
    break;
  case Package::EffectType:
    path.append("Effects");
    break;
  case Package::DataType:
    path.append("Data");
    break;
  case Package::ExtensionType:
    path.append("UserPlugins");
    break;
  case Package::ThemeType:
    path.append("ColorThemes");
    break;
  case Package::UnknownType:
    // The package has an unsupported type, so we return an empty path.
    // The empty path won't be used because the category will reject
    // this package right away. Maybe the parser should not bother with loading
    // unsupported packages at all anyway... But then in the future
    // we might want to display unsupported packages in the interface.
    return path;
  }

  // append the rest of the path
  switch(type) {
  case Package::ScriptType:
  case Package::EffectType:
    path.append(cat->index()->name());

    // only allow directory traversal up to the index name
    path += Path(cat->name()) + file();
    break;
  case Package::ExtensionType:
  case Package::ThemeType:
  case Package::DataType:
    path.append(file(), false);
    break;
  case Package::UnknownType:
    break; // will never happens, but compiler are dumb
  }

  return path;
}
