/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015  Christian Fillion
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

#include "version.hpp"

#include "errors.hpp"
#include "package.hpp"

#include <algorithm>
#include <cmath>
#include <regex>

using namespace std;

Version::Version(const std::string &str)
  : m_name(str), m_code(0), m_package(nullptr)
{
  static const regex pattern("(\\d+)");

  auto begin = sregex_iterator(str.begin(), str.end(), pattern);
  auto end = sregex_iterator();

  // set the major version by default
  // even if there are less than 4 numeric components in the string
  const size_t size = max((size_t)4, (size_t)distance(begin, end));

  if(begin == end || size > 4L)
    throw reapack_error("invalid version name");

  for(sregex_iterator it = begin; it != end; it++) {
    const string match = it->str(1);
    const size_t index = distance(begin, it);

    if(match.size() > 4)
      throw reapack_error("version component overflow");

    m_code += stoi(match) * (uint64_t)pow(10000, size - index - 1);
  }
}

Version::~Version()
{
  for(Source *source : m_sources)
    delete source;
}

string Version::fullName() const
{
  const string fName = "v" + m_name;
  return m_package ? m_package->fullName() + " " + fName : fName;
}

void Version::addSource(Source *source)
{
  const Source::Platform p = source->platform();

#ifdef __APPLE__
  if(p != Source::GenericPlatform && p < Source::DarwinPlatform)
    return;

#ifdef __x86_64__
  if(p == Source::Darwin32Platform)
    return;
#else
  if(p == Source::Darwin64Platform)
    return;
#endif

#elif _WIN32
  if(p == Source::UnknownPlatform || p > Source::Win64Platform)
    return;

#ifdef _WIN64
  if(p == Source::Win32Platform)
    return;
#else
  if(p == Source::Win64Platform)
    return;
#endif
#endif

  source->setVersion(this);
  m_sources.push_back(source);
}

void Version::setChangelog(const std::string &changelog)
{
  m_changelog = changelog;
}

vector<Path> Version::files() const
{
  const size_t size = m_sources.size();

  vector<Path> list(size);

  for(size_t i = 0; i < size; i++)
    list[i] = m_sources[i]->targetPath();

  return list;
}

bool Version::operator<(const Version &o) const
{
  return m_code < o.code();
}
