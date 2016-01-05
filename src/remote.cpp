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

#include "remote.hpp"

#include <fstream>
#include <regex>
#include <sstream>

#include "encoding.hpp"
#include "errors.hpp"

using namespace std;

static char DATA_DELIMITER = '|';

static bool ValidateName(const string &name)
{
  static const regex pattern("^[^~#%&*{}\\\\:<>?/+|\"]+$");

  smatch match;
  regex_match(name, match, pattern);

  return !match.empty();
}

static bool ValidateUrl(const string &url)
{
  using namespace std::regex_constants;

  // see http://tools.ietf.org/html/rfc3986#section-2
  static const regex pattern(
    "^(?:[a-z0-9._~:/?#[\\]@!$&'()*+,;=-]|%[a-f0-9]{2})+$", icase);

  smatch match;
  regex_match(url, match, pattern);

  return !match.empty();
}

Remote Remote::fromFile(const string &path, ReadCode *code)
{
  ifstream file(make_autostring(path));

  if(!file) {
    if(code)
      *code = ReadFailure;

    return {};
  }

  string name;
  getline(file, name);

  string url;
  getline(file, url);

  file.close();

  if(!ValidateName(name)) {
    if(code)
      *code = InvalidName;

    return {};
  }
  else if(!ValidateUrl(url)) {
    if(code)
      *code = InvalidUrl;

    return {};
  }

  if(code)
    *code = Success;

  return {name, url};
}

Remote Remote::fromString(const string &data, ReadCode *code)
{
  istringstream stream(data);

  string name;
  getline(stream, name, DATA_DELIMITER);

  string url;
  getline(stream, url, DATA_DELIMITER);

  string enabled;
  getline(stream, enabled, DATA_DELIMITER);

  if(!ValidateName(name)) {
    if(code)
      *code = InvalidName;
  }
  else if(!ValidateUrl(url)) {
    if(code)
      *code = InvalidUrl;
  }
  else {
    if(code)
      *code = Success;

    return {name, url, enabled == "1"};
  }

  return {};
}

Remote::Remote()
  : m_enabled(true), m_protected(false)
{
}

Remote::Remote(const string &name, const string &url, const bool enabled)
  : m_enabled(enabled), m_protected(false)
{
  setName(name);
  setUrl(url);
}

void Remote::setName(const string &name)
{
  if(!ValidateName(name))
    throw reapack_error("invalid name");
  else
    m_name = name;
}

void Remote::setUrl(const string &url)
{
  if(!ValidateUrl(url))
    throw reapack_error("invalid url");
  else
    m_url = url;
}

string Remote::toString() const
{
  return m_name + DATA_DELIMITER + m_url + DATA_DELIMITER + to_string(m_enabled);
}

void RemoteList::add(const Remote &remote)
{
  if(!remote.isValid())
    return;

  m_remotes[remote.name()] = remote;
}

Remote RemoteList::get(const string &name) const
{
  const auto it = m_remotes.find(name);

  if(it == m_remotes.end())
    return {};
  else
    return it->second;
}

vector<Remote> RemoteList::getEnabled() const
{
  vector<Remote> list;

  for(const auto &pair : m_remotes) {
    const Remote &remote = pair.second;

    if(remote.isEnabled())
      list.push_back(remote);
  }

  return list;
}
