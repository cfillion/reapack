/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2018  Christian Fillion
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

using namespace std;

string Version::displayAuthor(const string &author)
{
  if(author.empty())
    return "Unknown";
  else
    return author;
}

Version::Version(const string &str, const Package *pkg)
  : m_name(str), m_time(), m_package(pkg)
{
}

Version::~Version()
{
  for(const Source *source : m_sources)
    delete source;
}

string Version::fullName() const
{
  string name = m_package->fullName();
  name += " v";
  name += m_name.toString();

  return name;
}

bool Version::addSource(const Source *source)
{
  if(source->version() != this)
    throw reapack_error("source belongs to another version");
  else if(!source->platform().test())
    return false;

  const Path &path = source->targetPath();

  if(m_files.count(path))
    return false;

  m_files.insert(path);
  m_sources.push_back(source);

  return true;
}

ostream &operator<<(ostream &os, const Version &ver)
{
  os << 'v' << ver.name().toString();

  if(!ver.author().empty())
    os << " by " << ver.author();

  if(ver.time())
    os << " – " << ver.time();

  os << "\r\n";

  const string &changelog = ver.changelog();
  os << String::indent(changelog.empty() ? "No changelog" : changelog);

  return os;
}

VersionName::VersionName() : m_stable(true)
{}

VersionName::VersionName(const string &str)
{
  parse(str);
}

VersionName::VersionName(const VersionName &o)
  : m_string(o.m_string), m_segments(o.m_segments), m_stable(o.m_stable)
{
}

void VersionName::parse(const string &str)
{
  static const regex pattern("\\d+|[a-zA-Z]+");

  const auto &begin = sregex_iterator(str.begin(), str.end(), pattern);
  const sregex_iterator end;

  size_t letters = 0;
  vector<Segment> segments;

  for(sregex_iterator it = begin; it != end; it++) {
    const string &match = it->str(0);

    if(isalpha(match[0])) {
      if(segments.empty()) // got leading letters
        throw reapack_error(String::format("invalid version name '%s'", str.c_str()));

      segments.push_back(match);
      letters++;
    }
    else {
      try {
        segments.push_back(boost::lexical_cast<Numeric>(match));
      }
      catch(const boost::bad_lexical_cast &) {
        throw reapack_error(String::format("version segment overflow in '%s'", str.c_str()));
      }
    }
  }

  if(segments.empty()) // version doesn't have any numbers
    throw reapack_error(String::format("invalid version name '%s'", str.c_str()));

  m_string = str;
  swap(m_segments, segments);
  m_stable = letters < 1;
}

bool VersionName::tryParse(const string &str, string *errorOut)
{
  try {
    parse(str);
    return true;
  }
  catch(const reapack_error &err) {
    if(errorOut)
      *errorOut = err.what();

    return false;
  }
}

auto VersionName::segment(const size_t index) const -> Segment
{
  if(index < size())
    return m_segments[index];
  else
    return 0;
}

int VersionName::compare(const VersionName &o) const
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
    const Numeric *lnum = get_if<Numeric>(&lseg);
    const string *lstr = get_if<string>(&lseg);

    const Segment &rseg = o.segment(i);
    const Numeric *rnum = get_if<Numeric>(&rseg);
    const string *rstr = get_if<string>(&rseg);

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
