#include "package.hpp"

#include "database.hpp"
#include "errors.hpp"

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

VersionPtr Package::lastVersion() const
{
  return *prev(m_versions.end());
}

InstallLocation Package::targetLocation() const
{
  switch(m_type) {
  case ScriptType:
    return scriptLocation();
  default:
    throw reapack_error("unsupported package type");
  }
}

InstallLocation Package::scriptLocation() const
{
  // TODO: use actual database name instead of hard-coded "ReaScripts"
  InstallLocation loc("/Scripts/ReaScripts", name());

  if(m_category)
    loc.appendDir("/" + category()->name());

  return loc;
}
