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

  // read categories
  TiXmlElement *catNode = root->FirstChildElement();

  while(catNode) {
    db->addCategory(LoadCategoryV1(catNode));

    catNode = catNode->NextSiblingElement();
  }

  return db;
}

CategoryPtr LoadCategoryV1(TiXmlElement *catNode)
{
  if(strcmp(catNode->Value(), "category"))
    throw reapack_error("not a category");

  const char *name = catNode->Attribute("name");
  if(!name) name = "";

  CategoryPtr cat = make_shared<Category>(name);

  TiXmlElement *packNode = catNode->FirstChildElement();

  while(packNode) {
    cat->addPackage(LoadPackageV1(packNode));

    packNode = packNode->NextSiblingElement();
  }

  return cat;
}

PackagePtr LoadPackageV1(TiXmlElement *packNode)
{
  if(strcmp(packNode->Value(), "reapack"))
    throw reapack_error("not a package");

  const char *name = packNode->Attribute("name");
  if(!name) name = "";

  const char *type = packNode->Attribute("type");
  if(!type) type = "";

  const char *author = packNode->Attribute("author");
  if(!author) author = "";

  PackagePtr pack = make_shared<Package>(
    Package::convertType(type), name, author);

  TiXmlElement *verNode = packNode->FirstChildElement();

  while(verNode) {
    pack->addVersion(LoadVersionV1(verNode));

    verNode = verNode->NextSiblingElement();
  }

  return pack;
}

VersionPtr LoadVersionV1(TiXmlElement *verNode)
{
  if(strcmp(verNode->Value(), "version"))
    throw reapack_error("not a version");

  const char *name = verNode->Attribute("name");
  if(!name) name = "";

  VersionPtr ver = make_shared<Version>(name);

  return ver;
}
