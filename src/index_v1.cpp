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

#include <sstream>
#include <WDL/tinyxml/tinyxml.h>

using namespace std;

static void LoadMetadataV1(TiXmlElement *, Metadata *);
static void LoadCategoryV1(TiXmlElement *, Index *);
static void LoadPackageV1(TiXmlElement *, Category *);
static void LoadVersionV1(TiXmlElement *, Package *);
static void LoadSourceV1(TiXmlElement *, Version *);

void Index::loadV1(TiXmlElement *root, string *nameOut)
{
  if(nameOut) {
    if(const char *name = root->Attribute("name"))
      *nameOut = name;
  }

  TiXmlElement *node = root->FirstChildElement("category");

  while(node) {
    LoadCategoryV1(node, this);

    node = node->NextSiblingElement("category");
  }

  node = root->FirstChildElement("metadata");

  if(node)
    LoadMetadataV1(node, &m_metadata);
}

void LoadMetadataV1(TiXmlElement *meta, Metadata *md)
{
  TiXmlElement *node = meta->FirstChildElement("description");

  if(node) {
    if(const char *rtf = node->GetText())
      md->setAbout(rtf);
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

    md->addLink(Metadata::getLinkType(rel), {name, url});

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

  if(ri->addCategory(cat))
    ptr.release();
}

void LoadPackageV1(TiXmlElement *packNode, Category *cat)
{
  const char *type = packNode->Attribute("type");
  if(!type) type = "";

  const char *name = packNode->Attribute("name");
  if(!name) name = "";

  const char *desc = packNode->Attribute("desc");
  if(!desc) desc = "";

  Package *pack = new Package(Package::getType(type), name, cat);
  unique_ptr<Package> ptr(pack);

  pack->setDescription(desc);

  TiXmlElement *node = packNode->FirstChildElement("version");

  while(node) {
    LoadVersionV1(node, pack);

    node = node->NextSiblingElement("version");
  }

  node = packNode->FirstChildElement("metadata");

  if(node)
    LoadMetadataV1(node, pack->metadata());

  if(cat->addPackage(pack))
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
    LoadSourceV1(node, ver);
    node = node->NextSiblingElement("source");
  }

  node = verNode->FirstChildElement("changelog");

  if(node) {
    if(const char *changelog = node->GetText())
      ver->setChangelog(changelog);
  }

  if(pkg->addVersion(ver))
    ptr.release();
}

void LoadSourceV1(TiXmlElement *node, Version *ver)
{
  const char *platform = node->Attribute("platform");
  if(!platform) platform = "all";

  const char *type = node->Attribute("type");
  if(!type) type = "";

  const char *file = node->Attribute("file");
  if(!file) file = "";

  const char *main = node->Attribute("main");
  if(!main) main = "";

  const char *url = node->GetText();
  if(!url) url = "";

  Source *src = new Source(file, url, ver);
  unique_ptr<Source> ptr(src);

  src->setPlatform(platform);
  src->setTypeOverride(Package::getType(type));

  int sections = 0;
  string section;
  istringstream mainStream(main);
  while(getline(mainStream, section, '\x20'))
    sections |= Source::getSection(section.c_str());
  src->setSections(sections);

  if(ver->addSource(src))
    ptr.release();
}
