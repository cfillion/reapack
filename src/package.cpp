#include "package.hpp"

#include "database.hpp"
#include "errors.hpp"

#include <sstream>

using namespace std;

Package::Type Package::ConvertType(const char *type)
{
  if(!strcmp(type, "script"))
    return ScriptType;
  else
    return UnknownType;
}

Package::Package(const Type type, const string &name)
  : m_category(nullptr), m_type(type), m_name(name)
{
  if(m_name.empty())
    throw reapack_error("empty package name");
}

string Package::fullName() const
{
  return m_category ? m_category->fullName() + "/" + m_name : m_name;
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

  ver->setPackage(this);
  m_versions.insert(ver);
}

Version *Package::version(const size_t index) const
{
  auto it = m_versions.begin();

  for(size_t i = 0; i < index; i++)
    it++;

  return *it;
}

Version *Package::lastVersion() const
{
  if(m_versions.empty())
    return nullptr;

  return *m_versions.rbegin();
}

Path Package::targetPath() const
{
  Path path;

  if(!m_category || !m_category->database())
    throw reapack_error("category or database is unset");

  switch(m_type) {
  case ScriptType:
    path.append("Scripts");
    path.append(m_category->database()->name());
    path.append(m_category->name());
    break;
  default:
    throw reapack_error("unsupported package type");
  }

  return path;
}
