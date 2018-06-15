/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2018  Christian Fillion
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

#include "api.hpp"
#include "api_helper.hpp"

#include "about.hpp"
#include "errors.hpp"
#include "index.hpp"
#include "reapack.hpp"
#include "registry.hpp"
#include "remote.hpp"
#include "transaction.hpp"

using namespace std;

struct PackageEntry {
  Registry::Entry regEntry;
  vector<Registry::File> files;
};

static set<PackageEntry *> s_entries;

DEFINE_API(bool, AboutInstalledPackage, ((PackageEntry*, entry)),
R"(Show the about dialog of the given package entry.
The repository index is downloaded asynchronously if the cached copy doesn't exist or is older than one week.)",
{
  if(!s_entries.count(entry))
    return false;

  // the one given by the user may be deleted while we download the idnex
  const Registry::Entry entryCopy = entry->regEntry;

  const RemotePtr &remote = g_reapack->config()->remotes.getByName(entryCopy.remote);

  if(!remote)
    return false;

  return remote->fetchIndex([entryCopy] (const IndexPtr &index) {
    if(const Package *pkg = index->find(entryCopy.category, entryCopy.package))
      g_reapack->about()->setDelegate(make_shared<AboutPackageDelegate>(
        pkg, entryCopy.version));
  });

  return true;
});

DEFINE_API(bool, EnumOwnedFiles, ((PackageEntry*, entry))((int, index))
  ((char*, pathOut))((int, pathOut_sz))((int*, sectionsOut))((int*, typeOut)),
R"(Enumerate the files owned by the given package. Returns false when there is no more data.

sections: 0=not in action list, &1=main, &2=midi editor, &4=midi inline editor
type: see <a href="#ReaPack_GetEntryInfo">ReaPack_GetEntryInfo</a>.)",
{
  const size_t i = index;

  if(!s_entries.count(entry) || i >= entry->files.size())
    return false;

  const Registry::File &file = entry->files[i];
  if(pathOut)
    snprintf(pathOut, pathOut_sz, "%s", file.path.prependRoot().join().c_str());
  if(sectionsOut)
    *sectionsOut = file.sections;
  if(typeOut)
    *typeOut = (int)file.type;

  return entry->files.size() > i + 1;
});

DEFINE_API(bool, FreeEntry, ((PackageEntry*, entry)),
R"(Free resources allocated for the given package entry.)",
{
  if(!s_entries.count(entry))
    return false;

  s_entries.erase(entry);
  delete entry;
  return true;
});

DEFINE_API(bool, GetEntryInfo, ((PackageEntry*, entry))
  ((char*, repoOut))((int, repoOut_sz))((char*, catOut))((int, catOut_sz))
  ((char*, pkgOut))((int, pkgOut_sz))((char*, descOut))((int, descOut_sz))
  ((int*, typeOut))((char*, verOut))((int, verOut_sz))
  ((char*, authorOut))((int, authorOut_sz))
  ((bool*, pinnedOut))((int*, fileCountOut)),
R"(Get the repository name, category, package name, package description, package type, the currently installed version, author name, pinned status and how many files are owned by the given package entry.

type: 1=script, 2=extension, 3=effect, 4=data, 5=theme, 6=langpack, 7=webinterface)",
{
  if(!s_entries.count(entry))
    return false;

  const Registry::Entry &regEntry = entry->regEntry;

  if(repoOut)
    snprintf(repoOut, repoOut_sz, "%s", regEntry.remote.c_str());
  if(catOut)
    snprintf(catOut, catOut_sz, "%s", regEntry.category.c_str());
  if(pkgOut)
    snprintf(pkgOut, pkgOut_sz, "%s", regEntry.package.c_str());
  if(descOut)
    snprintf(descOut, descOut_sz, "%s", regEntry.description.c_str());
  if(typeOut)
    *typeOut = (int)regEntry.type;
  if(verOut)
    snprintf(verOut, verOut_sz, "%s", regEntry.version.toString().c_str());
  if(authorOut)
    snprintf(authorOut, authorOut_sz, "%s", regEntry.author.c_str());
  if(pinnedOut)
    *pinnedOut = regEntry.pinned;
  if(fileCountOut)
    *fileCountOut = (int)entry->files.size();

  return true;
});

DEFINE_API(PackageEntry*, GetOwner, ((const char*, fn))((char*, errorOut))((int, errorOut_sz)),
R"(Returns the package entry owning the given file.
Delete the returned object from memory after use with <a href="#ReaPack_FreeEntry">ReaPack_FreeEntry</a>.)",
{
  try {
    const Registry reg(Path::REGISTRY.prependRoot());
    const auto &owner = reg.getOwner(Path(fn).removeRoot());

    if(owner) {
      auto entry = new PackageEntry{owner, reg.getFiles(owner)};
      s_entries.insert(entry);
      return entry;
    }
    else if(errorOut)
      snprintf(errorOut, errorOut_sz, "the file is not owned by any package entry");

    return nullptr;
  }
  catch(const reapack_error &e)
  {
    if(errorOut)
      snprintf(errorOut, errorOut_sz, "%s", e.what());

    return nullptr;
  }
});
