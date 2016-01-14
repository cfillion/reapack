/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2016  Christian Fillion
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "registry.hpp"

#include "errors.hpp"
#include "package.hpp"
#include "path.hpp"

#include <reaper_plugin_functions.h>

using namespace std;

void Registry::push(Version *ver)
{
  Package *pkg = ver->package();

  if(!pkg)
    return;

  const Path id = pkg->targetPath() + pkg->name();
  push(id.join('/'), ver->name());
}

void Registry::push(const string &key, const string &value)
{
  m_map[key] = value;
}

Registry::QueryResult Registry::query(Package *pkg) const
{
  const Path id = pkg->targetPath() + pkg->name();
  const auto it = m_map.find(id.join('/'));

  if(it == m_map.end())
    return {Uninstalled, 0};

  Version *lastVer = pkg->lastVersion();
  const Status status = it->second == lastVer->name()
    ? UpToDate : UpdateAvailable;

  uint64_t versionCode = 0;

  try {
    if(status == UpdateAvailable)
      versionCode = Version(it->second).code();
    else
      versionCode = lastVer->code();
  }
  catch(const reapack_error &) {}

  return {status, versionCode};
}

bool Registry::addToREAPER(Version *ver, const Path &root)
{
  if(ver->package()->type() != Package::ScriptType)
    return false;

  Source *src = ver->mainSource();

  if(!src)
    return false;

  enum { MainSection = 0 };
  const string &path = (root + src->targetPath()).join();

  custom_action_register_t ca{MainSection, nullptr, path.c_str()};
  const int id = plugin_register("custom_action", (void *)&ca);

  return id > 0;
}
