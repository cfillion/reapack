#include "database.hpp"

#include "errors.hpp"

#include <WDL/tinyxml/tinyxml.h>

DatabasePtr Database::load(const char *file)
{
  TiXmlDocument doc(file);

  if(!doc.LoadFile())
    throw database_error("failed to read database");

  TiXmlHandle docHandle(&doc);
  TiXmlElement *root = doc.RootElement();

  if(strcmp(root->Value(), "index"))
    throw database_error("invalid database");

  int version = 0;
  root->Attribute("version", &version);

  if(!version)
    throw database_error("invalid version");

  switch(version) {
  case 1:
    return loadV1(root);
  default:
    throw database_error("unsupported version");
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
