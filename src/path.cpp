#include "path.hpp"

using namespace std;

#ifndef _WIN32
static const char SEPARATOR = '/';
#else
static const char SEPARATOR = '\\';
#endif

void Path::prepend(const string &part)
{
  m_parts.push_front(part);
}

void Path::append(const string &part)
{
  m_parts.push_back(part);
}

string Path::join(const bool skipLast) const
{
  string path;

  auto end = m_parts.end();

  if(skipLast)
    end--;

  for(auto it = m_parts.begin(); it != end; it++) {
    const string &part = *it;

    if(!path.empty())
      path.insert(path.end(), SEPARATOR);

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
