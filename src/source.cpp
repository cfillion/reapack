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
#include "package.hpp"
#include "version.hpp"

using namespace std;

Source::Platform Source::ConvertPlatform(const char *platform)
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

Source::Source(const Platform platform, const string &file,
    const string &url, Version *ver)
  : m_platform(platform), m_file(file), m_url(url), m_version(ver)
{
  if(m_url.empty())
    throw reapack_error("empty source url");
}

Package *Source::package() const
{
  return m_version ? m_version->package() : nullptr;
}

const string &Source::file() const
{
  if(!m_file.empty())
    return m_file;

  Package *pkg = package();

  if(pkg)
    return pkg->name();
  else
    throw reapack_error("empty file name and no package");
}

string Source::fullName() const
{
  if(!m_version)
    return file();
  else if(m_file.empty())
    return m_version->fullName();

  return m_version->fullName() + " (" + m_file + ")";
}

Path Source::targetPath() const
{
  Package *pkg = package();

  if(!pkg)
    throw reapack_error("no package associated with the source");

  return pkg->targetPath() + file();
}
