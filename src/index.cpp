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

#include "index.hpp"

#include "errors.hpp"
#include "filesystem.hpp"
#include "path.hpp"
#include "remote.hpp"

#include <WDL/tinyxml/tinyxml.h>

using namespace std;

Path Index::pathFor(const string &name)
{
  return Path::CACHE + (name + ".xml");
}

Index::Index(const RemotePtr &remote)
  : m_remote(remote)
{
}

Index::~Index()
{
  for(const Category *cat : m_categories)
    delete cat;
}

void Index::load()
{
  FILE *file = FS::open(pathFor(remote()->name()));

  if(!file)
    throw reapack_error(FS::lastError());

  TiXmlDocument doc;
  doc.LoadFile(file);
  fclose(file);

  doLoad(doc);
}

string Index::load(const char *data)
{
  TiXmlDocument doc;
  doc.Parse(data);

  string name;
  doLoad(doc, &name);
  return name;
}

void Index::doLoad(TiXmlDocument &doc, string *name)
{
  if(doc.ErrorId())
    throw reapack_error(doc.ErrorDesc());

  TiXmlHandle docHandle(&doc);
  TiXmlElement *root = doc.RootElement();

  if(strcmp(root->Value(), "index"))
    throw reapack_error("invalid index");

  int version = 0;
  root->Attribute("version", &version);

  if(!version)
    throw reapack_error("index version not found");

  switch(version) {
  case 1:
    loadV1(root, name);
    break;
  default:
    throw reapack_error("index version is unsupported");
  }
}

bool Index::addCategory(const Category *cat)
{
  if(cat->index() != this)
    throw reapack_error("category belongs to another index");

  if(cat->packages().empty())
    return false;

  m_catMap.insert({cat->name(), m_categories.size()});
  m_categories.push_back(cat);

  m_packages.insert(m_packages.end(),
    cat->packages().begin(), cat->packages().end());

  return true;
}

const Category *Index::category(const string &name) const
{
  const auto &it = m_catMap.find(name);

  if(it == m_catMap.end())
    return nullptr;
  else
    return category(it->second);
}

const Package *Index::find(const string &catName, const string &pkgName) const
{
  if(const Category *cat = category(catName))
    return cat->package(pkgName);
  else
    return nullptr;
}

Category::Category(const string &name, const Index *ri)
  : m_index(ri), m_name(name)
{
  if(m_name.empty())
    throw reapack_error("empty category name");
}

Category::~Category()
{
  for(const Package *pack : m_packages)
    delete pack;
}

string Category::fullName() const
{
  return m_index ? m_index->remote()->name() + "/" + m_name : m_name;
}

bool Category::addPackage(const Package *pkg)
{
  if(pkg->category() != this)
    throw reapack_error("package belongs to another category");

  if(pkg->type() == Package::UnknownType || pkg->versions().empty())
    return false; // silently discard unknown package types

  m_pkgMap.insert({pkg->name(), m_packages.size()});
  m_packages.push_back(pkg);
  return true;
}

const Package *Category::package(const string &name) const
{
  const auto &it = m_pkgMap.find(name);

  if(it == m_pkgMap.end())
    return nullptr;
  else
    return package(it->second);
}
