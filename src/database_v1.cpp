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

static void LoadCategoryV1(TiXmlElement *, Database *db);
static void LoadPackageV1(TiXmlElement *, Category *cat);
static void LoadVersionV1(TiXmlElement *, Package *pkg);

Database *Database::loadV1(TiXmlElement *root, const string &name)
{
  Database *db = new Database(name);

  // ensure the memory is released if an exception is
  // thrown during the loading process
  unique_ptr<Database> ptr(db);

  TiXmlElement *catNode = root->FirstChildElement("category");

  while(catNode) {
    LoadCategoryV1(catNode, db);

    catNode = catNode->NextSiblingElement("category");
  }

  ptr.release();
  return db;
}

void LoadCategoryV1(TiXmlElement *catNode, Database *db)
{
  const char *name = catNode->Attribute("name");
  if(!name) name = "";

  Category *cat = new Category(name, db);
  unique_ptr<Category> ptr(cat);

  TiXmlElement *packNode = catNode->FirstChildElement("reapack");

  while(packNode) {
    LoadPackageV1(packNode, cat);

    packNode = packNode->NextSiblingElement("reapack");
  }

  db->addCategory(cat);

  ptr.release();
}

void LoadPackageV1(TiXmlElement *packNode, Category *cat)
{
  const char *type = packNode->Attribute("type");
  if(!type) type = "";

  const char *name = packNode->Attribute("name");
  if(!name) name = "";

  Package *pack = new Package(Package::ConvertType(type), name, cat);
  unique_ptr<Package> ptr(pack);

  TiXmlElement *verNode = packNode->FirstChildElement("version");

  while(verNode) {
    LoadVersionV1(verNode, pack);

    verNode = verNode->NextSiblingElement("version");
  }

  cat->addPackage(pack);

  ptr.release();
}

void LoadVersionV1(TiXmlElement *verNode, Package *pkg)
{
  const char *name = verNode->Attribute("name");
  if(!name) name = "";

  Version *ver = new Version(name, pkg);
  unique_ptr<Version> ptr(ver);

  TiXmlElement *node = verNode->FirstChildElement("source");

  while(node) {
    const char *platform = node->Attribute("platform");
    if(!platform) platform = "all";

    const char *file = node->Attribute("file");
    if(!file) file = "";

    const char *url = node->GetText();
    if(!url) url = "";

    ver->addSource(new Source(Source::ConvertPlatform(platform), file, url, ver));

    node = node->NextSiblingElement("source");
  }

  node = verNode->FirstChildElement("changelog");

  if(node) {
    const char *changelog = node->GetText();

    if(changelog)
      ver->setChangelog(changelog);
  }

  pkg->addVersion(ver);

  ptr.release();
}
