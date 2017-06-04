/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2017  Christian Fillion
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

#include <boost/lexical_cast.hpp>
#include <boost/logic/tribool_io.hpp> // required to get correct tribool casts
#include <boost/mpl/aux_/preprocessor/token_equal.hpp>
#include <boost/preprocessor.hpp>

#include <reaper_plugin_functions.h>

#include "about.hpp"
#include "config.hpp"
#include "errors.hpp"
#include "index.hpp"
#include "reapack.hpp"
#include "registry.hpp"
#include "remote.hpp"
#include "transaction.hpp"

#define API_PREFIX "ReaPack_"

using namespace API;
using namespace std;

ReaPack *API::reapack = nullptr;

struct PackageEntry {
  Registry::Entry regEntry;
  vector<Registry::File> files;
};

static set<PackageEntry *> s_entries;

APIDef::APIDef(const APIFunc *func)
  : m_func(func)
{
  plugin_register(m_func->cKey, m_func->cImpl);
  plugin_register(m_func->reascriptKey, m_func->reascriptImpl);
  plugin_register(m_func->definitionKey, m_func->definition);
}

APIDef::~APIDef()
{
  unregister(m_func->cKey, m_func->cImpl);
  unregister(m_func->reascriptKey, m_func->reascriptImpl);
  unregister(m_func->definitionKey, m_func->definition);
}

void APIDef::unregister(const char *key, void *ptr)
{
  char buf[255];
  snprintf(buf, sizeof(buf), "-%s", key);
  plugin_register(buf, ptr);
}

#define BOOST_MPL_PP_TOKEN_EQUAL_void(x) x
#define IS_VOID(type) BOOST_MPL_PP_TOKEN_EQUAL(type, void)

#define ARG_TYPE(arg) BOOST_PP_TUPLE_ELEM(2, 0, arg)
#define ARG_NAME(arg) BOOST_PP_TUPLE_ELEM(2, 1, arg)

#define ARGS(r, data, i, arg) BOOST_PP_COMMA_IF(i) ARG_TYPE(arg) ARG_NAME(arg)
#define PARAMS(r, data, i, arg) BOOST_PP_COMMA_IF(i) (ARG_TYPE(arg))(intptr_t)argv[i]
#define DEFARGS(r, macro, i, arg) \
  BOOST_PP_EXPR_IF(i, ",") BOOST_PP_STRINGIZE(macro(arg))

#define DEFINE_API(type, name, args, help, ...) \
  namespace API_##name { \
    static type cImpl(BOOST_PP_SEQ_FOR_EACH_I(ARGS, _, args)) __VA_ARGS__ \
    static void *reascriptImpl(void **argv, int argc) { \
      BOOST_PP_EXPR_IF(BOOST_PP_NOT(IS_VOID(type)), return (void *)(intptr_t)) \
      cImpl(BOOST_PP_SEQ_FOR_EACH_I(PARAMS, _, args)); \
      BOOST_PP_EXPR_IF(IS_VOID(type), return nullptr;) \
    } \
    static const char *definition = #type "\0" \
      BOOST_PP_SEQ_FOR_EACH_I(DEFARGS, ARG_TYPE, args) "\0" \
      BOOST_PP_SEQ_FOR_EACH_I(DEFARGS, ARG_NAME, args) "\0" help; \
  }; \
  APIFunc API::name = {\
    "API_" API_PREFIX #name, (void *)&API_##name::cImpl, \
    "APIvararg_" API_PREFIX #name, (void *)&API_##name::reascriptImpl, \
    "APIdef_" API_PREFIX #name, (void *)API_##name::definition, \
  }

DEFINE_API(bool, AboutInstalledPackage, ((PackageEntry*, entry)),
R"(Show the about dialog of the given package entry.
The repository index is downloaded asynchronously if the cached copy doesn't exist or is older than one week.)",
{
  if(!s_entries.count(entry))
    return false;

  // the one given by the user may be deleted while we download the idnex
  const Registry::Entry entryCopy = entry->regEntry;

  const Remote &repo = reapack->remote(entryCopy.remote);
  if(!repo)
    return false;

  Transaction *tx = reapack->setupTransaction();
  if(!tx)
    return false;

  const vector<Remote> repos = {repo};

  tx->fetchIndexes(repos);
  tx->onFinish([=] {
    const auto &indexes = tx->getIndexes(repos);
    if(indexes.empty())
      return;

    const Package *pkg = indexes.front()->find(entryCopy.category, entryCopy.package);
    if(pkg)
      reapack->about()->setDelegate(make_shared<AboutPackageDelegate>(pkg, entryCopy.version));
  });
  tx->runTasks();

  return true;
});

DEFINE_API(bool, AboutRepository, ((const char*, repoName)),
R"(Show the about dialog of the given repository. Returns true if the repository exists in the user configuration.
The repository index is downloaded asynchronously if the cached copy doesn't exist or is older than one week.)",
{
  if(const Remote &repo = reapack->remote(repoName)) {
    reapack->about(repo);
    return true;
  }

  return false;
});

DEFINE_API(int, CompareVersions, ((const char*, ver1))((const char*, ver2))
    ((char*, errorOut))((int, errorOut_sz)),
R"(Returns 0 if both versions are equal, a positive value if ver1 is higher than ver2 and a negative value otherwise.)",
{
  VersionName a, b;
  string error;

  b.tryParse(ver2, &error);
  a.tryParse(ver1, &error);

  if(errorOut)
    snprintf(errorOut, errorOut_sz, "%s", error.c_str());

  return a.compare(b);
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
    snprintf(pathOut, pathOut_sz, "%s", Path::prefixRoot(file.path).join().c_str());
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
  Path path(fn);

  const Path &rp = ReaPack::resourcePath();

  if(path.startsWith(rp))
    path.remove(0, rp.size());

  try {
    const Registry reg(Path::prefixRoot(Path::REGISTRY));
    const auto &owner = reg.getOwner(path);

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

DEFINE_API(bool, GetRepositoryInfo, ((const char*, name))
  ((char*, urlOut))((int, urlOut_sz))
  ((bool*, enabledOut))((int*, autoInstallOut))((bool*, protectedOut)),
R"(Get the infos of the given repository.

autoInstall: 0=manual, 1=when sychronizing, 2=obey user setting)",
{
  const Remote &remote = reapack->remote(name);

  if(!remote)
    return false;

  if(urlOut)
    snprintf(urlOut, urlOut_sz, "%s", remote.url().c_str());
  if(enabledOut)
    *enabledOut = remote.isEnabled();
  if(autoInstallOut)
    *autoInstallOut = boost::lexical_cast<int>(remote.autoInstall());
  if(protectedOut)
    *protectedOut = remote.isProtected();

  return true;
});

DEFINE_API(bool, AddSetRepository, ((const char*, name))((const char*, url))
  ((bool, enabled))((int, autoInstall))((bool, commit))
  ((char*, errorOut))((int, errorOut_sz)),
R"(Add or modify a repository. Set commit to true for the last call to save the new list and update the GUI.

autoInstall: default is 2 (obey user setting).)",
{
  try {
    if(reapack->remote(name).isProtected()) {
      if(errorOut)
        snprintf(errorOut, errorOut_sz, "this repository is protected");
      return false;
    }

    Remote remote(name, url, enabled, boost::lexical_cast<tribool>(autoInstall));
    reapack->config()->remotes.add(remote);
  }
  catch(const reapack_error &e) {
    if(errorOut)
      snprintf(errorOut, errorOut_sz, "%s", e.what());
    return false;
  }
  catch(const boost::bad_lexical_cast &) {
    if(errorOut)
      snprintf(errorOut, errorOut_sz, "invalid value for autoInstall");
    return false;
  }

  if(commit) {
    reapack->refreshManager();
    reapack->refreshBrowser();
    reapack->config()->write();
  }

  return true;
});
