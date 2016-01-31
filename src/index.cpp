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

#include "index.hpp"

#include "encoding.hpp"
#include "errors.hpp"
#include "path.hpp"

#include <WDL/tinyxml/tinyxml.h>

using namespace std;

static FILE *OpenFile(const char *path)
{
  #ifdef _WIN32
  FILE *file = nullptr;
  _wfopen_s(&file, make_autostring(path).c_str(), L"rb");
  return file;
  #else
  return fopen(path, "rb");
  #endif
}

Path RemoteIndex::pathFor(const string &name)
{
  return Path::prefixCache(name + ".xml");
}

RemoteIndex *RemoteIndex::load(const string &name)
{
  TiXmlDocument doc;

  FILE *file = OpenFile(pathFor(name).join().c_str());

  const bool success = doc.LoadFile(file);

  if(file)
    fclose(file);

  if(!success)
    throw reapack_error("failed to read index");

  TiXmlHandle docHandle(&doc);
  TiXmlElement *root = doc.RootElement();

  if(strcmp(root->Value(), "index"))
    throw reapack_error("invalid index");

  int version = 0;
  root->Attribute("version", &version);

  if(!version)
    throw reapack_error("invalid version");

  switch(version) {
  case 1:
    return loadV1(root, name);
  default:
    throw reapack_error("unsupported version");
  }
}

RemoteIndex::RemoteIndex(const string &name)
  : m_name(name)
{
  if(m_name.empty())
    throw reapack_error("empty index name");
}

RemoteIndex::~RemoteIndex()
{
  for(Category *cat : m_categories)
    delete cat;
}

void RemoteIndex::addCategory(Category *cat)
{
  if(cat->index() != this)
    throw reapack_error("category belongs to another index");

  if(cat->packages().empty())
    return;

  m_categories.push_back(cat);

  m_packages.insert(m_packages.end(),
    cat->packages().begin(), cat->packages().end());
}

Category::Category(const string &name, RemoteIndex *ri)
  : m_index(ri), m_name(name)
{
  if(m_name.empty())
    throw reapack_error("empty category name");
}

Category::~Category()
{
  for(Package *pack : m_packages)
    delete pack;
}

string Category::fullName() const
{
  return m_index ? m_index->name() + "/" + m_name : m_name;
}

void Category::addPackage(Package *pkg)
{
  if(pkg->category() != this)
    throw reapack_error("package belongs to another category");

  if(pkg->type() == Package::UnknownType)
    return; // silently discard unknown package types
  else if(pkg->versions().empty())
    return;

  m_packages.push_back(pkg);
}
