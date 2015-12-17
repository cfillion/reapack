/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015  Christian Fillion
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

#include "database.hpp"

#include "errors.hpp"

#include <WDL/tinyxml/tinyxml.h>

using namespace std;

Database *Database::load(const char *file)
{
  TiXmlDocument doc(file);

  if(!doc.LoadFile())
    throw reapack_error("failed to read database");

  TiXmlHandle docHandle(&doc);
  TiXmlElement *root = doc.RootElement();

  if(strcmp(root->Value(), "index"))
    throw reapack_error("invalid database");

  int version = 0;
  root->Attribute("version", &version);

  if(!version)
    throw reapack_error("invalid version");

  switch(version) {
  case 1:
    return loadV1(root);
  default:
    throw reapack_error("unsupported version");
  }
}

Database::~Database()
{
  for(Category *cat : m_categories)
    delete cat;
}

void Database::addCategory(Category *cat)
{
  if(cat->packages().empty())
    return;

  cat->setDatabase(this);
  m_categories.push_back(cat);

  m_packages.insert(m_packages.end(),
    cat->packages().begin(), cat->packages().end());
}

Category::Category(const string &name)
  : m_database(nullptr), m_name(name)
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
  return m_database ? m_database->name() + "/" + m_name : m_name;
}

void Category::addPackage(Package *pack)
{
  if(pack->type() == Package::UnknownType)
    return; // silently discard unknown package types
  else if(pack->versions().empty())
    return;

  pack->setCategory(this);
  m_packages.push_back(pack);
}
