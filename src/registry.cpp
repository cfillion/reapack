#include "registry.hpp"

#include "package.hpp"
#include "path.hpp"

using namespace std;

void Registry::push(Package *pkg)
{
  push(pkg->targetPath().join(), pkg->lastVersion()->name());
}

void Registry::push(const std::string &key, const std::string &value)
{
  m_map.insert({key, value});
}

string Registry::versionOf(Package *pkg) const
{
  const string key = pkg->targetPath().join();

  const auto it = m_map.find(key);
  if(it == m_map.end())
    return std::string();

  return it->second;
}
