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

Package::Package(const Type type, const std::string &name)
  : m_category(0), m_type(type), m_name(name)
{
  if(m_name.empty())
    throw reapack_error("empty package name");
}

Package::~Package()
{
  for(Version *ver : m_versions)
    delete ver;
}

void Package::addVersion(Version *ver)
{
  if(ver->sources().empty())
    return;

  m_versions.insert(ver);
}

Version *Package::version(const int index) const
{
  auto it = m_versions.begin();

  for(int i = 0; i < index; i++)
    it++;

  return *it;
}

Version *Package::lastVersion() const
{
  return *prev(m_versions.end());
}

Path Package::targetLocation() const
{
  switch(m_type) {
  case ScriptType:
    return scriptLocation();
  default:
    throw reapack_error("unsupported package type");
  }
}

Path Package::scriptLocation() const
{
  Path path;
  path.append("Scripts");

  if(!m_category || !m_category->database())
    throw reapack_error("category or database is unset");

  path.append(m_category->database()->name());
  path.append(m_category->name());
  path.append(m_name);

  return path;
}
