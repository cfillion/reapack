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

#include <boost/preprocessor.hpp>

#include <reaper_plugin_functions.h>

#include "about.hpp"
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

static set<Registry::Entry *> s_entries;

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

#define ARG_TYPE(arg) BOOST_PP_TUPLE_ELEM(2, 0, arg)
#define ARG_NAME(arg) BOOST_PP_TUPLE_ELEM(2, 1, arg)

#define ARGS(r, data, i, arg) BOOST_PP_COMMA_IF(i) ARG_TYPE(arg) ARG_NAME(arg)
#define PARAMS(r, data, i, arg) BOOST_PP_COMMA_IF(i) (ARG_TYPE(arg))(intptr_t)argv[i]
#define DEFARGS(r, macro, i, arg) \
  BOOST_PP_EXPR_IF(i, ",") BOOST_PP_STRINGIZE(macro(arg))

#define DEFINE_API(type, name, args, help, ...) \
  namespace API_##name { \
    static type cImpl(BOOST_PP_SEQ_FOR_EACH_I(ARGS, _, args)) __VA_ARGS__ \
    void *reascriptImpl(void **argv, int argc) { \
      return (void *)(intptr_t)cImpl(BOOST_PP_SEQ_FOR_EACH_I(PARAMS, _, args)); \
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

DEFINE_API(bool, AboutInstalledPackage, ((Registry::Entry*, entry)), R"(
  Show the about dialog of the given package entry.
  The repository index is downloaded asynchronously if the cached copy doesn't exist or is older than one week.
)", {
  if(!s_entries.count(entry))
    return false;

  // the one given by the user may be deleted while we download the idnex
  const Registry::Entry entryCopy = *entry;

  const Remote &repo = reapack->remote(entry->remote);
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

DEFINE_API(bool, AboutRepository, ((const char*, repoName)), R"(
  Show the about dialog of the given repository.
  The repository index is downloaded asynchronously if the cached copy doesn't exist or is older than one week.
)", {
  if(const Remote &repo = reapack->remote(repoName)) {
    reapack->about(repo);
    return true;
  }

  return false;
});

DEFINE_API(bool, CompareVersions, ((const char*, ver1))((const char*, ver2))
    ((int*, resultOut))((char*, errorOut))((int, errorOut_sz)), R"(
  Returns 0 if both versions are equal, a positive value if ver1 is higher than ver2 and a negative value otherwise.
)", {
  VersionName a, b;
  string error;

  if(a.tryParse(ver1, &error) && b.tryParse(ver2, &error)) {
    *resultOut = a.compare(b);
    return true;
  }

  if(errorOut)
    snprintf(errorOut, errorOut_sz, "%s", error.c_str());

  *resultOut = 0;

  return false;
});

DEFINE_API(Registry::Entry*, GetOwner, ((const char*, fn))((char*, errorOut))((int, errorOut_sz)), R"(
  Returns the package entry owning the given file.
  Delete the returned object from memory after use with <a href="#ReaPack_FreeEntry">ReaPack_FreeEntry</a>.
)", {
  Path path(fn);

  const Path &rp = ReaPack::resourcePath();

  if(path.startsWith(rp))
    path.remove(0, rp.size());

  try {
    const Registry reg(Path::prefixRoot(Path::REGISTRY));
    const auto &owner = reg.getOwner(path);

    if(owner) {
      auto entry = new Registry::Entry(owner);
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

DEFINE_API(bool, FreeEntry, ((Registry::Entry*, entry)), R"(
  Free resources allocated for the given package entry.
)", {
  if(!s_entries.count(entry))
    return false;

  s_entries.erase(entry);
  delete entry;
  return true;
});
