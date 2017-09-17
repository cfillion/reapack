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

#include "browser.hpp"
#include "reapack.hpp"

using namespace std;

DEFINE_API(void, BrowsePackages, ((const char*, filter)),
R"(Opens the package browser with the given filter string.)",
{
  if(Browser *browser = g_reapack->browsePackages())
    browser->setFilter(filter);
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

DEFINE_API(void, ProcessQueue, ((bool, refreshUI)),
R"(Run pending operations and save the configuration file. If refreshUI is true the browser and manager windows are guaranteed to be refreshed (otherwise it depends on which operations are in the queue).)",
{
  g_reapack->commitConfig(refreshUI);
});
