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

using namespace std;

Source::Platform Source::getPlatform(const char *platform)
{
  if(!strcmp(platform, "all"))
    return GenericPlatform;
  else if(!strcmp(platform, "windows"))
    return WindowsPlatform;
  else if(!strcmp(platform, "win32"))
    return Win32Platform;
  else if(!strcmp(platform, "win64"))
    return Win64Platform;
  else if(!strcmp(platform, "darwin"))
    return DarwinPlatform;
  else if(!strcmp(platform, "darwin32"))
    return Darwin32Platform;
  else if(!strcmp(platform, "darwin64"))
    return Darwin64Platform;
  else
    return UnknownPlatform;
}

Source::Source(const string &file, const string &url, const Version *ver)
  : m_platform(GenericPlatform), m_type(Package::UnknownType),
    m_file(file), m_url(url), m_version(ver)
{
  if(m_url.empty())
    throw reapack_error("empty source url");
}

const Package *Source::package() const
{
  return m_version ? m_version->package() : nullptr;
}

const string &Source::file() const
{
  if(!m_file.empty())
    return m_file;

  const Package *pkg = package();

  if(pkg)
    return pkg->name();
  else
    throw reapack_error("empty file name and no package");
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

  const Package::Type type = m_type ? m_type : pkg->type();

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
  case Package::DataType:
    path.append(cat->index()->name());

    // only allow directory traversal up to the index name
    path += Path(cat->name()) + file();
    break;
  case Package::ExtensionType:
    path.append(file(), false);
    break;
  case Package::UnknownType:
    break; // will never happens, but compiler are dumb
  }

  return path;
}
