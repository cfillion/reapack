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
#include "api_helper.hpp"

#include "errors.hpp"
#include "reapack.hpp"
#include "remote.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/logic/tribool_io.hpp> // required to get correct tribool casts

using namespace std;

DEFINE_API(bool, AboutRepository, ((const char*, repoName)),
R"(Show the about dialog of the given repository. Returns true if the repository exists in the user configuration.
The repository index is downloaded asynchronously if the cached copy doesn't exist or is older than one week.)",
{
  if(const Remote &repo = g_reapack->remote(repoName)) {
    g_reapack->about(repo);
    return true;
  }

  return false;
});

DEFINE_API(bool, AddSetRepository, ((const char*, name))((const char*, url))
  ((bool, enable))((int, autoInstall))((bool, commit))
  ((char*, errorOut))((int, errorOut_sz)),
R"(Add or modify a repository. Set url to nullptr (or empty string in Lua) to keep the existing URL. Set commit to true for the last call to process the new list and update the GUI.

autoInstall: usually set to 2 (obey user setting).)",
{
  try {
    const Remote &existing = g_reapack->remote(name);

    if(!url || strlen(url) == 0)
      url = existing.url().c_str();

    if(existing.isProtected() && url != existing.url()) {
      if(errorOut)
        snprintf(errorOut, errorOut_sz, "cannot change URL of a protected repository");
      return false;
    }

    Remote remote = g_reapack->remote(name);
    remote.setName(name);
    remote.setUrl(url);
    remote.setAutoInstall(boost::lexical_cast<tribool>(autoInstall));

    if(!g_reapack->addSetRemote(remote, enable))
      return false;
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

  if(commit)
    g_reapack->commitConfig();

  return true;
});

DEFINE_API(bool, GetRepositoryInfo, ((const char*, name))
  ((char*, urlOut))((int, urlOut_sz))
  ((bool*, enabledOut))((int*, autoInstallOut)),
R"(Get the infos of the given repository.

autoInstall: 0=manual, 1=when sychronizing, 2=obey user setting)",
{
  const Remote &remote = g_reapack->remote(name);

  if(!remote)
    return false;

  if(urlOut)
    snprintf(urlOut, urlOut_sz, "%s", remote.url().c_str());
  if(enabledOut)
    *enabledOut = remote.isEnabled();
  if(autoInstallOut)
    *autoInstallOut = boost::lexical_cast<int>(remote.autoInstall());

  return true;
});
