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

#include "version.hpp"

#include "errors.hpp"
#include "package.hpp"

#include <algorithm>
#include <cmath>
#include <regex>

using namespace std;

Version::Version()
  : m_code(0), m_time(), m_package(nullptr), m_mainSource(nullptr)
{
}

Version::Version(const string &str, const Package *pkg)
  : m_code(0), m_time(), m_package(pkg), m_mainSource(nullptr)
{
  parse(str);
}

Version::Version(const Version &o, const Package *pkg)
  : m_name(o.name()), m_code(o.code()),
    m_author(o.author()), m_changelog(o.changelog()), m_time(o.time()),
    m_package(pkg), m_mainSource(nullptr)
{
}

Version::~Version()
{
  for(const auto &pair : m_sources)
    delete pair.second;
}

void Version::parse(const string &str)
{
  static const regex pattern("(\\d+)");

  auto begin = sregex_iterator(str.begin(), str.end(), pattern);
  auto end = sregex_iterator();

  // set the major version by default
  // even if there are less than 4 numeric components in the string
  const size_t size = max((size_t)4, (size_t)distance(begin, end));

  if(begin == end || size > 4L)
    throw reapack_error("invalid version name");

  size_t index = 0;

  for(sregex_iterator it = begin; it != end; it++, index++) {
    const string match = it->str(1);

    if(match.size() > 4)
      throw reapack_error("version component overflow");

    m_code += stoi(match) * (Code)pow(10000, size - index - 1);
  }

  m_name = str;
}

bool Version::tryParse(const string &str)
{
  try {
    parse(str);
    return true;
  }
  catch(const reapack_error &) {
    return false;
  }
}

string Version::fullName() const
{
  const string fName = 'v' + m_name;
  return m_package ? m_package->fullName() + " " + fName : fName;
}

string Version::displayAuthor() const
{
  if(m_author.empty())
    return "Unknown";
  else
    return m_author;
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

  if(source->version() != this)
    throw reapack_error("source belongs to another version");

  const Path path = source->targetPath();
  m_files.insert(path);
  m_sources.insert({path, source});

  if(source->isMain())
    m_mainSource = source;
}

void Version::setChangelog(const string &changelog)
{
  m_changelog = changelog;
}

void Version::setTime(const char *iso8601)
{
  unsigned int year, month, day;
  size_t n = sscanf(iso8601, "%u-%u-%u", &year, &month, &day);

  if(n < 3 || year < 1900 || month < 1 || month > 12 || day < 1 || day > 31)
    return;

  m_time = {};
  m_time.tm_year = year - 1900;
  m_time.tm_mon = month - 1;
  m_time.tm_mday = day;
}

string Version::displayTime() const
{
  if(m_time.tm_year == 0)
    return {};

  char buf[32] = {};
  strftime(buf, sizeof(buf), "%B %d %Y", &m_time);
  return buf;
}

const Source *Version::source(const size_t index) const
{
  auto it = m_sources.begin();

  for(size_t i = 0; i < index; i++)
    it++;

  return it->second;
}

bool Version::operator<(const Version &o) const
{
  return m_code < o.code();
}

bool Version::operator<=(const Version &o) const
{
  return !(m_code > o.code());
}

bool Version::operator>(const Version &o) const
{
  return m_code > o.code();
}

bool Version::operator>=(const Version &o) const
{
  return !(m_code < o.code());
}

bool Version::operator==(const Version &o) const
{
  return m_code == o.code();
}

bool Version::operator!=(const Version &o) const
{
  return !(*this == o);
}
