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

#include "source.hpp"

#include "errors.hpp"
#include "index.hpp"

#include <boost/algorithm/string/case_conv.hpp>

using namespace std;

auto Source::getSection(const char *name) -> Section
{
  if(!strcmp(name, "main"))
    return MainSection;
  else if(!strcmp(name, "midi_editor"))
    return MIDIEditorSection;
  else if(!strcmp(name, "midi_inlineeditor"))
    return MIDIInlineEditorSection;
  else if(!strcmp(name, "midi_eventlisteditor"))
    return MIDIEventListEditorSection;
  else if(!strcmp(name, "mediaexplorer"))
    return MediaExplorerSection;
  else if(!strcmp(name, "true"))
    return ImplicitSection;
  else
    return UnknownSection;
}

auto Source::detectSection(const string &category) -> Section
{
  // this is for compatibility with indexes made for v1.0

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

Package::Type Source::type() const
{
  if(m_type)
    return m_type;
  else
    return m_version->package()->type();
}

const string &Source::file() const
{
  if(!m_file.empty())
    return m_file;
  else
    return m_version->package()->name();
}

void Source::setSections(int sections)
{
  if(type() != Package::ScriptType)
    return;
  else if(sections == ImplicitSection)
    sections = detectSection(m_version->package()->category()->name());

  m_sections = sections;
}

Path Source::targetPath() const
{
  Path path;
  const auto type = this->type();

  // select the target directory
  switch(type) {
  case Package::ScriptType:
    path += "Scripts";
    break;
  case Package::EffectType:
    path += "Effects";
    break;
  case Package::DataType:
    path += "Data";
    break;
  case Package::ExtensionType:
    path += "UserPlugins";
    break;
  case Package::ThemeType:
    path += "ColorThemes";
    break;
  case Package::LangPackType:
    path += "LangPack";
    break;
  case Package::WebInterfaceType:
    path += "reaper_www_root";
    break;
  case Package::ProjectTemplateType:
    path += "ProjectTemplates";
    break;
  case Package::TrackTemplateType:
    path += "TrackTemplates";
    break;
  case Package::MIDINoteNamesType:
    path += "MIDINoteNames";
    break;
  case Package::AutomationItemType:
    path += "AutomationItems";
    break;
  case Package::UnknownType:
    // The package has an unsupported type, so we make an empty path.
    // The empty path won't be used because the category will reject
    // this package right away. Maybe the parser should not bother with loading
    // unsupported packages at all anyway... But then in the future
    // we might want to display unsupported packages in the interface.
    break;
  }

  // append the rest of the path
  switch(type) {
  case Package::ScriptType:
  case Package::EffectType:
  case Package::AutomationItemType: {
    const Category *cat = m_version->package()->category();
    path += cat->index()->name();

    // only allow directory traversal up to the index name
    path += Path(cat->name()) + file();
    break;
  }
  case Package::ExtensionType:
  case Package::ThemeType:
  case Package::DataType:
  case Package::LangPackType:
  case Package::WebInterfaceType:
  case Package::ProjectTemplateType:
  case Package::TrackTemplateType:
  case Package::MIDINoteNamesType:
    path.append(file(), false); // no directory traversal
    break;
  case Package::UnknownType:
    break;
  }

  return path;
}
