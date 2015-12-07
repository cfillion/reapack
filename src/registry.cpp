#include "registry.hpp"

#include "package.hpp"
#include "path.hpp"

using namespace std;

void Registry::push(Package *pkg)
{
  Version *lastVer = pkg->lastVersion();

  if(!lastVer)
    return;

  push(pkg->targetPath().join(), lastVer->name());
}

void Registry::push(const std::string &key, const std::string &value)
{
  m_map[key] = value;
}

Registry::Status Registry::query(Package *pkg) const
{
  const string key = pkg->targetPath().join();

  const auto it = m_map.find(key);
  if(it == m_map.end())
    return Uninstalled;

  return it->second == pkg->lastVersion()->name() ? UpToDate : UpdateAvailable;
}
