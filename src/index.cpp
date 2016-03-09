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

#include "download.hpp"
#include "encoding.hpp"
#include "errors.hpp"
#include "filesystem.hpp"
#include "path.hpp"
#include "remote.hpp"

#include <boost/algorithm/string/predicate.hpp>

#include <WDL/tinyxml/tinyxml.h>

using namespace std;

Path Index::pathFor(const string &name)
{
  return Path::CACHE + (name + ".xml");
}

auto Index::linkTypeFor(const char *rel) -> LinkType
{
  if(!strcmp(rel, "donation"))
    return DonationLink;

  return WebsiteLink;
}

IndexPtr Index::load(const string &name, const char *data)
{
  TiXmlDocument doc;

  if(data)
    doc.Parse(data);
  else {
    FILE *file = FS::open(pathFor(name));

    if(!file)
      throw reapack_error(FS::lastError().c_str());

    doc.LoadFile(file);
    fclose(file);
  }

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

  Index *ri = new Index(name);

  // ensure the memory is released if an exception is
  // thrown during the loading process
  unique_ptr<Index> ptr(ri);

  switch(version) {
  case 1:
    loadV1(root, ri);
    break;
  default:
    throw reapack_error("index version is unsupported");
  }

  ptr.release();
  return IndexPtr(ri);
}

Download *Index::fetch(const Remote &remote, const bool stale)
{
  time_t mtime = 0, now = time(nullptr);

  if(FS::mtime(pathFor(remote.name()), &mtime)) {
    const time_t threshold = stale ? 2 : (7 * 24 * 3600);

    if(mtime > now - threshold)
      return nullptr;
  }

  return new Download(remote.name() + ".xml", remote.url());
}

Index::Index(const string &name)
  : m_name(name)
{
}

Index::~Index()
{
  for(const Category *cat : m_categories)
    delete cat;
}

void Index::setName(const string &newName)
{
  if(!m_name.empty())
    throw reapack_error("index name is already set");

  // validation is taken care of later by Remote's constructor
  m_name = newName;
}

void Index::addCategory(const Category *cat)
{
  if(cat->index() != this)
    throw reapack_error("category belongs to another index");

  if(cat->packages().empty())
    return;

  m_categories.push_back(cat);

  m_packages.insert(m_packages.end(),
    cat->packages().begin(), cat->packages().end());
}

void Index::addLink(const LinkType type, const Link &link)
{
  if(boost::algorithm::starts_with(link.url, "http"))
    m_links.insert({type, link});
}

auto Index::links(const LinkType type) const -> LinkList
{
  const auto begin = m_links.lower_bound(type);
  const auto end = m_links.upper_bound(type);

  LinkList list(m_links.count(type));

  for(auto it = begin; it != end; it++)
    list[distance(begin, it)] = &it->second;

  return list;
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
  return m_index ? m_index->name() + "/" + m_name : m_name;
}

void Category::addPackage(const Package *pkg)
{
  if(pkg->category() != this)
    throw reapack_error("package belongs to another category");

  if(pkg->type() == Package::UnknownType)
    return; // silently discard unknown package types
  else if(pkg->versions().empty())
    return;

  m_packages.push_back(pkg);
}
