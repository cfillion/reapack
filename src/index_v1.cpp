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

#include "errors.hpp"

#include <WDL/tinyxml/tinyxml.h>

using namespace std;

static void LoadMetadataV1(TiXmlElement *, Index *ri);
static void LoadCategoryV1(TiXmlElement *, Index *ri);
static void LoadPackageV1(TiXmlElement *, Category *cat);
static void LoadVersionV1(TiXmlElement *, Package *pkg);

void Index::loadV1(TiXmlElement *root, Index *ri)
{
  if(ri->name().empty()) {
    if(const char *name = root->Attribute("name"))
      ri->setName(name);
  }

  TiXmlElement *node = root->FirstChildElement("category");

  while(node) {
    LoadCategoryV1(node, ri);

    node = node->NextSiblingElement("category");
  }

  node = root->FirstChildElement("metadata");

  if(node)
    LoadMetadataV1(node, ri);
}

void LoadMetadataV1(TiXmlElement *meta, Index *ri)
{
  TiXmlElement *node = meta->FirstChildElement("description");

  if(node) {
    if(const char *rtf = node->GetText())
      ri->setAboutText(rtf);
  }

  node = meta->FirstChildElement("link");

  while(node) {
    const char *rel = node->Attribute("rel");
    const char *url = node->Attribute("href");
    const char *name = node->GetText();

    if(!rel) rel = "";
    if(!name) {
      if(!url) url = "";
      name = url;
    }
    else if(!url) url = name;

    ri->addLink(Index::linkTypeFor(rel), {name, url});

    node = node->NextSiblingElement("link");
  }
}

void LoadCategoryV1(TiXmlElement *catNode, Index *ri)
{
  const char *name = catNode->Attribute("name");
  if(!name) name = "";

  Category *cat = new Category(name, ri);
  unique_ptr<Category> ptr(cat);

  TiXmlElement *packNode = catNode->FirstChildElement("reapack");

  while(packNode) {
    LoadPackageV1(packNode, cat);

    packNode = packNode->NextSiblingElement("reapack");
  }

  ri->addCategory(cat);

  ptr.release();
}

void LoadPackageV1(TiXmlElement *packNode, Category *cat)
{
  const char *type = packNode->Attribute("type");
  if(!type) type = "";

  const char *name = packNode->Attribute("name");
  if(!name) name = "";

  Package *pack = new Package(Package::typeFor(type), name, cat);
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

  const char *author = verNode->Attribute("author");
  if(author) ver->setAuthor(author);

  const char *time = verNode->Attribute("time");
  if(time) ver->setTime(time);

  TiXmlElement *node = verNode->FirstChildElement("source");

  while(node) {
    const char *platform = node->Attribute("platform");
    if(!platform) platform = "all";

    const char *file = node->Attribute("file");
    if(!file) file = "";

    const char *url = node->GetText();
    if(!url) url = "";

    Source *src = new Source(file, url, ver);
    src->setPlatform(Source::ConvertPlatform(platform));
    ver->addSource(src);

    node = node->NextSiblingElement("source");
  }

  node = verNode->FirstChildElement("changelog");

  if(node) {
    if(const char *changelog = node->GetText())
      ver->setChangelog(changelog);
  }

  pkg->addVersion(ver);

  ptr.release();
}
