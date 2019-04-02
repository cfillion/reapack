/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2019  Christian Fillion
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

static constexpr char UNIX_SEPARATOR = '/';

#ifndef _WIN32
static constexpr char NATIVE_SEPARATOR = UNIX_SEPARATOR;
#else
#include <windows.h> // MAX_PATH
static constexpr char NATIVE_SEPARATOR = '\\';
#endif

static constexpr const char *DOT = ".";
static constexpr const char *DOTDOT = "..";

const Path Path::DATA("ReaPack");
const Path Path::CACHE = Path::DATA + "cache";
const Path Path::CONFIG("reapack.ini");
const Path Path::REGISTRY = Path::DATA + "registry.db";

Path Path::s_root;

static std::vector<std::string> Split(const std::string &input, bool *absolute)
{
  std::vector<std::string> list;

  const auto append = [&list, absolute] (const std::string &part) {
    if(part.empty() || part == DOT)
      return;

#ifdef _WIN32
    if(list.empty() && part.size() == 2 && isalpha(part[0]) && part[1] == ':')
      *absolute = true;
#else
    (void)absolute;
#endif

    list.push_back(part);
  };

  size_t last = 0, size = input.size();

  while(last < size) {
    const size_t pos = input.find_first_of("\\/", last);

    if(pos == std::string::npos) {
      append(input.substr(last));
      break;
    }
    else if(last + pos == 0) {
#ifndef _WIN32
      *absolute = true;
#endif
      last++;
      continue;
    }

    append(input.substr(last, pos - last));

    last = pos + 1;
  }

  return list;
}

Path::Path(const std::string &path) : m_absolute(false)
{
  append(path);
}

void Path::append(const std::string &input, const bool traversal)
{
  if(input.empty())
    return;

  bool absolute = false;
  const auto &parts = Split(input, &absolute);

  if(m_parts.empty() && absolute)
    m_absolute = true;

  for(const std::string &part : parts) {
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
  if(m_parts.empty() && o.absolute())
    m_absolute = true;

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
  std::advance(begin, pos);

  auto end = begin;
  std::advance(end, count);

  m_parts.erase(begin, end);

  if(!pos && m_absolute)
    m_absolute = false;
}

void Path::removeLast()
{
  if(!empty())
    m_parts.pop_back();
}

std::string Path::front() const
{
  if(empty())
    return {};

  return m_parts.front();
}

std::string Path::basename() const
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

std::string Path::join(const bool nativeSeparator) const
{
  const char sep = nativeSeparator ? NATIVE_SEPARATOR : UNIX_SEPARATOR;

#ifdef _WIN32
  constexpr bool absoluteSlash = false;
#else
  const bool absoluteSlash = m_absolute;
#endif

  std::string path;

  for(const std::string &part : m_parts) {
    if(!path.empty() || absoluteSlash)
      path += sep;

    path += part;
  }

  if(m_parts.empty() && absoluteSlash)
    path += sep;

#ifdef _WIN32
  if(m_absolute && path.size() > MAX_PATH)
    path.insert(0, "\\\\?\\");
#endif

  return path;
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

Path Path::prependRoot() const
{
  return m_absolute ? *this : s_root + *this;
}

Path Path::removeRoot() const
{
  Path copy(*this);

  if(startsWith(s_root))
    copy.remove(0, s_root.size());

  return copy;
}

bool Path::operator==(const Path &o) const
{
  return m_absolute == o.absolute() && m_parts == o.m_parts;
}

bool Path::operator!=(const Path &o) const
{
  return !(*this == o);
}

bool Path::operator<(const Path &o) const
{
  return m_parts < o.m_parts;
}

Path Path::operator+(const std::string &part) const
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

const Path &Path::operator+=(const std::string &parts)
{
  append(parts);
  return *this;
}

const Path &Path::operator+=(const Path &o)
{
  append(o);
  return *this;
}

const std::string &Path::at(const size_t index) const
{
  auto it = m_parts.begin();
  advance(it, index);

  return *it;
}

std::string &Path::operator[](const size_t index)
{
  return const_cast<std::string &>(at(index));
}

const std::string &Path::operator[](const size_t index) const
{
  return at(index);
}

UseRootPath::UseRootPath(const Path &path)
  : m_backup(std::move(Path::s_root))
{
  Path::s_root = path;
}

UseRootPath::~UseRootPath()
{
  Path::s_root = std::move(m_backup);
}

TempPath::TempPath(const Path &target)
  : m_target(target), m_temp(target)
{
  m_temp[m_temp.size() - 1] += ".part";
}
