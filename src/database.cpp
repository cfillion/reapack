#include "database.hpp"

#include "errors.hpp"

#include <WDL/tinyxml/tinyxml.h>

DatabasePtr Database::load(const char *file)
{
  TiXmlDocument doc(file);

  if(!doc.LoadFile())
    throw reapack_error("failed to read database");

  TiXmlHandle docHandle(&doc);
  TiXmlElement *root = doc.RootElement();

  if(strcmp(root->Value(), "index"))
    throw reapack_error("invalid database");

  int version = 0;
  root->Attribute("version", &version);

  if(!version)
    throw reapack_error("invalid version");

  switch(version) {
  case 1:
    return loadV1(root);
  default:
    throw reapack_error("unsupported version");
  }
}

void Database::addCategory(CategoryPtr cat)
{
  if(cat->packages().empty())
    return;

  m_categories.push_back(cat);
}

Category::Category(const std::string &name)
  : m_name(name)
{
  if(m_name.empty())
    throw reapack_error("empty category name");
}

void Category::addPackage(PackagePtr pack)
{
  if(pack->type() == Package::UnknownType)
    return; // silently discard unknown package types
  else if(pack->versions().empty())
    return;

  pack->setCategory(this);
  m_packages.push_back(pack);
}

Package::Type Package::convertType(const char *type)
{
  if(!strcmp(type, "script"))
    return ScriptType;
  else
    return UnknownType;
}

Package::Package(const Type type,
    const std::string &name, const std::string &author)
  : m_category(0), m_type(type), m_name(name), m_author(author)
{
  if(m_name.empty())
    throw reapack_error("empty package name");

  if(m_author.empty())
    throw reapack_error("empty package author");
}

void Package::addVersion(VersionPtr ver)
{
  if(ver->sources().empty())
    return;

  m_versions.insert(ver);
}

VersionPtr Package::version(const int index) const
{
  auto it = m_versions.begin();

  for(int i = 0; i < index; i++)
    it++;

  return *it;
}
