#include "database.hpp"

#include <WDL/tinyxml/tinyxml.h>

DatabasePtr Database::load(const char *file, const char **error)
{
  TiXmlDocument doc(file);

  if(!doc.LoadFile()) {
    *error = "failed to read database";
    return 0;
  }

  TiXmlHandle docHandle(&doc);
  TiXmlElement *root = doc.RootElement();

  if(strcmp(root->Value(), "index")) {
    *error = "invalid database";
    return 0;
  }

  int version = 0;
  root->Attribute("version", &version);

  if(!version) {
    *error = "invalid version";
    return 0;
  }

  switch(version) {
  case 1:
    return loadV1(root, error);
  default:
    *error = "unsupported version";
    return 0;
  }
}

void Database::addCategory(CategoryPtr cat)
{
  m_categories.push_back(cat);
}

Category::Category(const std::string &name)
  : m_name(name)
{
}

void Category::addPackage(PackagePtr pack)
{
  pack->setCategory(this);
  m_packages.push_back(pack);
}

Package::Type Package::convertType(const char *type)
{
  if(!type)
    return UnknownType;

  if(!strcmp(type, "script"))
    return ScriptType;

  return UnknownType;
}

Package::Package()
  : m_category(0)
{
}
