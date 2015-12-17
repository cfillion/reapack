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

#include <memory>

using namespace std;

static Category *LoadCategoryV1(TiXmlElement *);
static Package *LoadPackageV1(TiXmlElement *);
static Version *LoadVersionV1(TiXmlElement *);

Database *Database::loadV1(TiXmlElement *root)
{
  Database *db = new Database;

  // ensure the memory is released if an exception is
  // thrown during the loading process
  unique_ptr<Database> ptr(db);

  TiXmlElement *catNode = root->FirstChildElement("category");

  while(catNode) {
    db->addCategory(LoadCategoryV1(catNode));

    catNode = catNode->NextSiblingElement("category");
  }

  ptr.release();
  return db;
}

Category *LoadCategoryV1(TiXmlElement *catNode)
{
  const char *name = catNode->Attribute("name");
  if(!name) name = "";

  Category *cat = new Category(name);
  unique_ptr<Category> ptr(cat);

  TiXmlElement *packNode = catNode->FirstChildElement("reapack");

  while(packNode) {
    cat->addPackage(LoadPackageV1(packNode));

    packNode = packNode->NextSiblingElement("reapack");
  }

  ptr.release();
  return cat;
}

Package *LoadPackageV1(TiXmlElement *packNode)
{
  const char *type = packNode->Attribute("type");
  if(!type) type = "";

  const char *name = packNode->Attribute("name");
  if(!name) name = "";

  Package *pack = new Package(Package::ConvertType(type), name);
  unique_ptr<Package> ptr(pack);

  TiXmlElement *verNode = packNode->FirstChildElement("version");

  while(verNode) {
    pack->addVersion(LoadVersionV1(verNode));

    verNode = verNode->NextSiblingElement("version");
  }

  ptr.release();
  return pack;
}

Version *LoadVersionV1(TiXmlElement *verNode)
{
  const char *name = verNode->Attribute("name");
  if(!name) name = "";

  Version *ver = new Version(name);
  unique_ptr<Version> ptr(ver);

  TiXmlElement *node = verNode->FirstChildElement("source");

  while(node) {
    const char *platform = node->Attribute("platform");
    if(!platform) platform = "all";

    const char *file = node->Attribute("file");
    if(!file) file = "";

    const char *url = node->GetText();
    if(!url) url = "";

    ver->addSource(new Source(Source::ConvertPlatform(platform), file, url));

    node = node->NextSiblingElement("source");
  }

  node = verNode->FirstChildElement("changelog");

  if(node) {
    const char *changelog = node->GetText();

    if(changelog)
      ver->setChangelog(changelog);
  }

  ptr.release();
  return ver;
}
