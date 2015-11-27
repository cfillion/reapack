#include "database.hpp"

#include "errors.hpp"

#include <WDL/tinyxml/tinyxml.h>

using namespace std;

static CategoryPtr LoadCategoryV1(TiXmlElement *);
static PackagePtr LoadPackageV1(TiXmlElement *);
static VersionPtr LoadVersionV1(TiXmlElement *);

DatabasePtr Database::loadV1(TiXmlElement *root)
{
  DatabasePtr db = make_shared<Database>();

  TiXmlElement *catNode = root->FirstChildElement("category");

  while(catNode) {
    db->addCategory(LoadCategoryV1(catNode));

    catNode = catNode->NextSiblingElement("category");
  }

  return db;
}

CategoryPtr LoadCategoryV1(TiXmlElement *catNode)
{
  const char *name = catNode->Attribute("name");
  if(!name) name = "";

  CategoryPtr cat = make_shared<Category>(name);

  TiXmlElement *packNode = catNode->FirstChildElement("reapack");

  while(packNode) {
    cat->addPackage(LoadPackageV1(packNode));

    packNode = packNode->NextSiblingElement("reapack");
  }

  return cat;
}

PackagePtr LoadPackageV1(TiXmlElement *packNode)
{
  const char *name = packNode->Attribute("name");
  if(!name) name = "";

  const char *type = packNode->Attribute("type");
  if(!type) type = "";

  const char *author = packNode->Attribute("author");
  if(!author) author = "";

  PackagePtr pack = make_shared<Package>(
    Package::convertType(type), name, author);

  TiXmlElement *verNode = packNode->FirstChildElement("version");

  while(verNode) {
    pack->addVersion(LoadVersionV1(verNode));

    verNode = verNode->NextSiblingElement("version");
  }

  return pack;
}

VersionPtr LoadVersionV1(TiXmlElement *verNode)
{
  const char *name = verNode->Attribute("name");
  if(!name) name = "";

  VersionPtr ver = make_shared<Version>(name);

  TiXmlElement *node = verNode->FirstChildElement("source");

  while(node) {
    const char *platform = node->Attribute("platform");
    if(!platform) platform = "all";

    const char *url = node->GetText();
    if(!url) url = "";

    ver->addSource(make_shared<Source>(Source::convertPlatform(platform), url));

    node = node->NextSiblingElement("source");
  }

  return ver;
}
