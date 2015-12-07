#include "registry.hpp"

#include "errors.hpp"
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

Registry::QueryResult Registry::query(Package *pkg) const
{
  const string key = pkg->targetPath().join();
  const auto it = m_map.find(key);

  if(it == m_map.end())
    return {Uninstalled, 0};

  Version *lastVer = pkg->lastVersion();
  const Status status = it->second == lastVer->name()
    ? UpToDate : UpdateAvailable;

  size_t versionCode = 0;

  try {
    if(status == UpdateAvailable)
      versionCode = Version(it->second).code();
    else
      versionCode = lastVer->code();
  }
  catch(const reapack_error &) {}

  return {status, versionCode};
}
