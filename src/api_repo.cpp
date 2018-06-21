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

#include "config.hpp"
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
  if(const RemotePtr &repo = g_reapack->config()->remotes.getByName(repoName)) {
    repo->about();
    return true;
  }

  return false;
});

DEFINE_API(bool, AddSetRepository, ((const char*, name))((const char*, url))
  ((bool, enable))((int, autoInstall))((char*, errorOut))((int, errorOut_sz)),
R"(Add or modify a repository. Set url to nullptr (or empty string in Lua) to keep the existing URL. Call <a href="#ReaPack_ProcessQueue">ReaPack_ProcessQueue(true)</a> when done to process the new list and update the GUI.

autoInstall: usually set to 2 (obey user setting).)",
try {
  RemoteList &remotes = g_reapack->config()->remotes;
  RemotePtr remote, existing = remotes.getByName(name);

  if(existing) {
    remote = make_shared<Remote>(*existing);

    if(url && strlen(url) > 0)
      remote->setUrl(url);
  }
  else {
    remote = make_shared<Remote>(name, url);
    remote->setDirty();
  }

  remote->setEnabled(enable);
  remote->setAutoInstall(boost::lexical_cast<boost::tribool>(autoInstall));

  if(existing)
    swap(*remote, *existing); // edit the main instance after all validation
  else
    remotes.add(remote);

  return true;
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
});

DEFINE_API(bool, GetRepositoryInfo, ((const char*, name))
  ((char*, urlOut))((int, urlOut_sz))
  ((bool*, enabledOut))((int*, autoInstallOut)),
R"(Get the infos of the given repository.

autoInstall: 0=manual, 1=when sychronizing, 2=obey user setting)",
{
  const RemotePtr &remote = g_reapack->config()->remotes.getByName(name);

  if(!remote)
    return false;

  if(urlOut)
    snprintf(urlOut, urlOut_sz, "%s", remote->url().c_str());
  if(enabledOut)
    *enabledOut = remote->isEnabled();
  if(autoInstallOut)
    *autoInstallOut = boost::lexical_cast<int>(remote->autoInstall());

  return true;
});
