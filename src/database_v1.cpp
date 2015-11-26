#include "database.hpp"

#include "errors.hpp"

#include <WDL/tinyxml/tinyxml.h>

using namespace std;

DatabasePtr Database::loadV1(TiXmlElement *root)
{
  DatabasePtr db = make_shared<Database>();

  // read categories
  TiXmlElement *catNode = root->FirstChildElement();

  while(catNode) {
    db->addCategory(Category::loadV1(catNode));

    catNode = catNode->NextSiblingElement();
  }

  return db;
}

CategoryPtr Category::loadV1(TiXmlElement *catNode)
{
  if(strcmp(catNode->Value(), "category"))
    throw database_error("not a category");

  const char *name = catNode->Attribute("name");
  if(!name) name = "";

  CategoryPtr cat = make_shared<Category>(name);

  TiXmlElement *packNode = catNode->FirstChildElement();

  while(packNode) {
    cat->addPackage(Package::loadV1(packNode));

    packNode = packNode->NextSiblingElement();
  }

  return cat;
}

PackagePtr Package::loadV1(TiXmlElement *packNode)
{
  if(strcmp(packNode->Value(), "reapack"))
    throw database_error("not a package");

  const char *name = packNode->Attribute("name");
  if(!name) name = "";

  const char *type = packNode->Attribute("type");
  if(!type) type = "";

  const char *author = packNode->Attribute("author");
  if(!author) author = "";

  return make_shared<Package>(Package::convertType(type), name, author);
}
