#include "database.hpp"

#include <WDL/tinyxml/tinyxml.h>

using namespace std;

DatabasePtr Database::loadV1(TiXmlElement *root, const char **error)
{
  DatabasePtr db = make_shared<Database>();

  // read categories
  TiXmlElement *catNode = root->FirstChildElement();

  while(catNode) {
    CategoryPtr cat = Category::loadV1(catNode, error);

    if(!cat.get())
      return 0;

    db->addCategory(cat);

    catNode = catNode->NextSiblingElement();
  }

  return db;
}

CategoryPtr Category::loadV1(TiXmlElement *catNode, const char **error)
{
  if(strcmp(catNode->Value(), "category")) {
    *error = "not a category";
    return 0;
  }

  const char *name = catNode->Attribute("name");

  if(!name || !strlen(name)) {
    *error = "missing category name";
    return 0;
  }

  CategoryPtr cat = make_shared<Category>(name);

  TiXmlElement *packNode = catNode->FirstChildElement();

  while(packNode) {
    PackagePtr pack = Package::loadV1(packNode, error);

    if(!pack.get())
      return 0;

    cat->addPackage(pack);

    packNode = packNode->NextSiblingElement();
  }
  return cat;
}

PackagePtr Package::loadV1(TiXmlElement *packNode, const char **error)
{
  if(strcmp(packNode->Value(), "reapack")) {
    *error = "not a package";
    return 0;
  }

  const char *name = packNode->Attribute("name");

  if(!name || !strlen(name)) {
    *error = "missing package name";
    return 0;
  }

  const char *typeStr = packNode->Attribute("type");
  const Package::Type type = Package::convertType(typeStr);

  if(type == Package::UnknownType) {
    *error = "unsupported package type";
    return 0;
  }

  const char *author = packNode->Attribute("author");

  if(!author || !strlen(author)) {
    *error = "package is anonymous";
    return 0;
  }

  return 0;
}
