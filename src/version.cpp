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
#include "source.hpp"

#include <boost/lexical_cast.hpp>
#include <cctype>
#include <regex>

using boost::format;
using namespace std;

Version::Version()
  : m_stable(true), m_time(), m_package(nullptr)
{
}

Version::Version(const string &str, const Package *pkg)
  : m_time(), m_package(pkg)
{
  parse(str);
}

Version::Version(const Version &o, const Package *pkg)
  : m_name(o.m_name), m_segments(o.m_segments), m_stable(o.m_stable),
    m_author(o.m_author), m_changelog(o.m_changelog), m_time(o.m_time),
    m_package(pkg)
{
}

Version::~Version()
{
  for(const auto &pair : m_sources)
    delete pair.second;
}

void Version::parse(const string &str)
{
  static const regex pattern("\\d+|[a-zA-Z]+");

  const auto &begin = sregex_iterator(str.begin(), str.end(), pattern);
  const sregex_iterator end;

  size_t letters = 0;
  vector<Segment> segments;

  for(sregex_iterator it = begin; it != end; it++) {
    const string match = it->str(0);
    const char first = tolower(match[0]);

    if(first >= 'a' || first >= 'z') {
      if(segments.empty()) // got leading letters
        throw reapack_error(format("invalid version name '%s'") % str);

      segments.push_back(match);
      letters++;
    }
    else {
      try {
        segments.push_back(boost::lexical_cast<Numeric>(match));
      }
      catch(const boost::bad_lexical_cast &) {
        throw reapack_error(format("version segment overflow in '%s'") % str);
      }
    }
  }

  if(segments.empty()) // version doesn't have any numbers
    throw reapack_error(format("invalid version name '%s'") % str);

  m_name = str;
  swap(m_segments, segments);
  m_stable = letters < 1;
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

bool Version::addSource(const Source *source)
{
  if(source->version() != this)
    throw reapack_error("source belongs to another version");
  else if(!source->platform().test())
    return false;

  const Path path = source->targetPath();
  m_files.insert(path);
  m_sources.insert({path, source});

  if(source->isMain())
    m_mainSources.push_back(source);

  return true;
}

const Source *Version::source(const size_t index) const
{
  auto it = m_sources.begin();

  for(size_t i = 0; i < index; i++)
    it++;

  return it->second;
}

auto Version::segment(const size_t index) const -> Segment
{
  if(index < size())
    return m_segments[index];
  else
    return 0;
}

int Version::compare(const Version &o) const
{
  const size_t biggest = max(size(), o.size());

  switch(m_segments.empty() + o.m_segments.empty()) {
  case 1:
    return m_segments.empty() ? -1 : 1;
  case 2:
    return 0;
  }

  for(size_t i = 0; i < biggest; i++) {
    const Segment &lseg = segment(i);
    const Numeric *lnum = boost::get<Numeric>(&lseg);
    const string *lstr = boost::get<string>(&lseg);

    const Segment &rseg = o.segment(i);
    const Numeric *rnum = boost::get<Numeric>(&rseg);
    const string *rstr = boost::get<string>(&rseg);

    if(lnum && rnum) {
      if(*lnum < *rnum)
        return -1;
      else if(*lnum > *rnum)
        return 1;
    }
    else if(lstr && rstr) {
      if(*lstr < *rstr)
        return -1;
      else if(*lstr > *rstr)
        return 1;
    }
    else if(lnum && rstr)
      return 1;
    else if(lstr && rnum)
      return -1;
  }

  return 0;
}
