#include "database.hpp"

#include <WDL/tinyxml/tinyxml.h>

DatabasePtr Database::loadV1(TiXmlElement *root, const char **error)
{
  DatabasePtr db = std::make_shared<Database>();

  // read categories
  TiXmlElement *catNode = root->FirstChildElement("category");

  while(catNode) {
    const char *name = catNode->Attribute("name");

    if(!name) {
      *error = "missing category name";
      return 0;
    }

    catNode = catNode->NextSiblingElement();
  }

  return db;
}
