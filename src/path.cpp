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

#include "path.hpp"

using namespace std;

#ifndef _WIN32
static const char SEPARATOR = '/';
#else
static const char SEPARATOR = '\\';
#endif

void Path::prepend(const string &part)
{
  if(!part.empty())
    m_parts.push_front(part);
}

void Path::append(const string &part)
{
  if(!part.empty())
    m_parts.push_back(part);
}

void Path::clear()
{
  m_parts.clear();
}

string Path::basename() const
{
  if(m_parts.empty())
    return {};

  return m_parts.back();
}

string Path::join(const bool skipLast, const char sep) const
{
  string path;

  auto end = m_parts.end();

  if(skipLast)
    end--;

  for(auto it = m_parts.begin(); it != end; it++) {
    const string &part = *it;

    if(!path.empty())
      path.insert(path.end(), sep ? sep : SEPARATOR);

    path.append(part);
  }

  return path;
}

bool Path::operator==(const Path &o) const
{
  return m_parts == o.m_parts;
}

bool Path::operator!=(const Path &o) const
{
  return !(*this == o);
}

bool Path::operator<(const Path &o) const
{
  return m_parts < o.m_parts;
}

Path Path::operator+(const string &part) const
{
  Path path(*this);
  path.append(part);

  return path;
}

Path Path::operator+(const Path &o) const
{
  Path path(*this);
  path.m_parts.insert(path.m_parts.end(), o.m_parts.begin(), o.m_parts.end());

  return path;
}

string &Path::operator[](const size_t index)
{
  auto it = m_parts.begin();

  for(size_t i = 0; i < index; i++)
    it++;

  return *it;
}
