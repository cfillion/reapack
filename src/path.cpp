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
    return string();

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
