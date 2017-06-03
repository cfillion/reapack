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

#include "path.hpp"

#include <algorithm>
#include <boost/range/adaptor/reversed.hpp>
#include <vector>

using namespace std;

#ifndef _WIN32
static const char SEPARATOR = '/';
#else
static const char SEPARATOR = '\\';
#endif

static const string DOT = ".";
static const string DOTDOT = "..";

Path Path::DATA = Path("ReaPack");
Path Path::CACHE = Path::DATA + "cache";
Path Path::CONFIG = Path("reapack.ini");
Path Path::REGISTRY = Path::DATA + "registry.db";

Path Path::s_root;

static vector<string> Split(const string &input, bool *absolute)
{
  vector<string> list;

  size_t last = 0, size = input.size();

  while(last < size) {
    const size_t pos = input.find_first_of("\\/", last);

    if(pos == string::npos) {
      list.push_back(input.substr(last));
      break;
    }
    else if(last + pos == 0) {
      *absolute = true;
      last++;
      continue;
    }

    const string part = input.substr(last, pos - last);

    if(!part.empty() && part != ".")
      list.push_back(part);

    last = pos + 1;
  }

  return list;
}

Path::Path(const string &path) : m_absolute(false)
{
  append(path);
}

void Path::append(const string &input, const bool traversal)
{
  if(input.empty())
    return;

  bool absolute = false;
  const auto &parts = Split(input, &absolute);

  if(m_parts.empty() && absolute)
    m_absolute = true;

  for(const string &part : parts) {
    if(part == DOTDOT) {
      if(traversal)
        removeLast();
    }
    else
      m_parts.push_back(part);
  }
}

void Path::append(const Path &o)
{
  m_parts.insert(m_parts.end(), o.m_parts.begin(), o.m_parts.end());
}

void Path::clear()
{
  m_parts.clear();
}

void Path::remove(const size_t pos, size_t count)
{
  if(pos > size())
    return;
  else if(pos + count > size())
    count = size() - pos;

  auto begin = m_parts.begin();
  advance(begin, pos);

  auto end = begin;
  advance(end, count);

  m_parts.erase(begin, end);

  if(!pos && m_absolute)
    m_absolute = false;
}

void Path::removeLast()
{
  if(!empty())
    m_parts.pop_back();
}

string Path::basename() const
{
  if(empty())
    return {};

  return m_parts.back();
}

Path Path::dirname() const
{
  if(empty())
    return {};

  Path dir(*this);
  dir.removeLast();
  return dir;
}

string Path::join(const char sep) const
{
  string path;

  for(const string &part : m_parts) {
    if(!path.empty() || m_absolute)
      path += sep ? sep : SEPARATOR;

    path += part;
  }

  return path;
}

string Path::first() const
{
  if(empty())
    return {};

  return m_parts.front();
}

string Path::last() const
{
  if(empty())
    return {};

  return m_parts.back();
}

bool Path::startsWith(const Path &o) const
{
  if(size() < o.size() || absolute() != o.absolute())
    return false;

  for(size_t i = 0; i < o.size(); i++) {
    if(o[i] != at(i))
      return false;
  }

  return true;
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
  path.append(o);

  return path;
}

const Path &Path::operator+=(const string &parts)
{
  append(parts);
  return *this;
}

const Path &Path::operator+=(const Path &o)
{
  append(o);
  return *this;
}

const string &Path::at(const size_t index) const
{
  auto it = m_parts.begin();
  advance(it, index);

  return *it;
}

string &Path::operator[](const size_t index)
{
  return const_cast<string &>(at(index));
}

const string &Path::operator[](const size_t index) const
{
  return at(index);
}

UseRootPath::UseRootPath(const string &path)
  : m_backup(move(Path::s_root))
{
  Path::s_root = path;
}

UseRootPath::~UseRootPath()
{
  Path::s_root = move(m_backup);
}

TempPath::TempPath(const Path &target)
  : m_target(target), m_temp(target)
{
  m_temp[m_temp.size() - 1] += ".part";
}
