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

#include "receipt.hpp"

#include "index.hpp"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <sstream>

using namespace std;

Receipt::Receipt()
  : m_flags(NoFlag)
{
}

bool Receipt::empty() const
{
  return
    m_installs.empty() &&
    m_removals.empty() &&
    m_exports.empty() &&
    m_errors.empty();
}

void Receipt::addInstall(const Version *ver, const Registry::Entry &entry)
{
  m_installs.emplace(InstallTicket{ver, entry});
  m_flags |= InstalledFlag;

  if(ver->package()->type() == Package::ExtensionType)
    m_flags |= RestartNeededFlag;
}

void Receipt::addRemoval(const Path &path)
{
  m_removals.insert(path);
  m_flags |= RemovedFlag;
}

void Receipt::addExport(const Path &path)
{
  m_exports.insert(path);
  m_flags |= ExportedFlag;
}

void Receipt::addError(const ErrorInfo &err)
{
  m_errors.push_back(err);
  m_flags |= ErrorFlag;
}

ReceiptPage Receipt::installedPage() const
{
  return {m_installs, "Installed"};
}

ReceiptPage Receipt::removedPage() const
{
  return {m_removals, "Removed"};
}

ReceiptPage Receipt::exportedPage() const
{
  return {m_exports, "Exported"};
}

ReceiptPage Receipt::errorPage() const
{
  return {m_errors, "Error", "Errors"};
}

void ReceiptPage::setTitle(const char *title)
{
  m_title = String::format("%s (%s)", title, String::number(m_size).c_str());
}

InstallTicket::InstallTicket(const Version *ver, const Registry::Entry &previous)
  : m_version(ver), m_previous(previous.version), m_type(deduceType(previous))
{
  m_index = ver->package()->category()->index()->shared_from_this();
}

InstallTicket::Type InstallTicket::deduceType(const Registry::Entry &previous) const
{
  if(!previous)
    return Install;
  else if(previous.version < m_version->name())
    return Update;
  else if(previous.version > m_version->name())
    return Downgrade;
  else
    return Reinstall;
}

bool InstallTicket::operator<(const InstallTicket &o) const
{
  string l = m_version->package()->displayName(),
    r = o.m_version->package()->displayName();

  boost::algorithm::to_lower(l);
  boost::algorithm::to_lower(r);

  return l < r;
}

ostream &operator<<(ostream &os, const InstallTicket &t)
{
  if(os.tellp() > 0)
    os << "\r\n";

  os << t.m_version->package()->fullName();

  switch(t.m_type) {
  case InstallTicket::Install:
    os << " [new]";
    break;
  case InstallTicket::Reinstall:
    os << " [reinstalled]";
    break;
  case InstallTicket::Update:
  case InstallTicket::Downgrade:
    os << " [v" << t.m_previous.toString() << " -> v" << t.m_version->name().toString() << ']';
    break;
  }

  if(t.m_type == InstallTicket::Update) {
    const auto &versions = t.m_version->package()->versions();

    for(const Version *ver : versions | boost::adaptors::reversed) {
      if(ver->name() <= t.m_previous)
        break;
      else if(ver->name() <= t.m_version->name())
        os << "\r\n" << *ver;
    }
  }
  else
    os << "\r\n" << *t.m_version;

  return os;
}
