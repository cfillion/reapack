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

#include "report.hpp"

#include "package.hpp"
#include "resource.hpp"
#include "source.hpp"
#include "string.hpp"
#include "transaction.hpp"

#include <boost/range/adaptor/reversed.hpp>

using namespace std;

Report::Report(const Receipt &receipt)
  : Dialog(IDD_REPORT_DIALOG), m_receipt(receipt)
{
}

void Report::onInit()
{
  Dialog::onInit();

  fillReport();

  SetFocus(getControl(IDOK));
}

void Report::printHeader(const Char *title)
{
  if(m_stream.tellp())
    m_stream << "\r\n";

  const String sep(10, L'=');
  m_stream << sep << L' ' << title << L": " << sep << "\r\n\r\n";
}

void Report::fillReport()
{
  const size_t installs = m_receipt.installs().size();
  const size_t updates = m_receipt.updates().size();
  const size_t removals = m_receipt.removals().size();
  const size_t errors = m_receipt.errors().size();

  m_stream << installs << L" installed package";
  if(installs != 1) m_stream << 's';

  m_stream << L", " << updates << L" update";
  if(updates != 1) m_stream << L's';

  m_stream << L", " << removals << L" removed file";
  if(removals != 1) m_stream << 's';

  m_stream << L" and " << errors << L" error";
  if(errors != 1) m_stream << L's';

  m_stream << L"\r\n";

  if(m_receipt.isRestartNeeded()) {
    m_stream
      << L"\r\n"
      << L"Notice: One or more native REAPER extensions were installed.\r\n"
      << L"The newly installed files won't be loaded until REAPER is restarted."
      << L"\r\n";
  }

  if(errors)
    printErrors();

  if(installs)
    printInstalls();

  if(updates)
    printUpdates();

  if(removals)
    printRemovals();

  if(installs + updates + removals == 0) {
    SetDlgItemText(handle(), IDC_LABEL,
      L"Oops! The following error(s) occured:");
  }

  SetDlgItemText(handle(), IDC_REPORT, m_stream.str().c_str());
}

void Report::printInstalls()
{
  printHeader(L"Installed packages");

  for(const InstallTicket &ticket : m_receipt.installs())
    m_stream << ticket.version->fullName() << "\r\n";
}

void Report::printUpdates()
{
  printHeader(L"Updates");

  const auto start = m_stream.tellp();

  for(const InstallTicket &ticket : m_receipt.updates()) {
    const Package *pkg = ticket.version->package();
    const auto &versions = pkg->versions();

    if(m_stream.tellp() != start)
      m_stream << L"\r\n";

    m_stream << pkg->fullName() << L":\r\n";

    for(const Version *ver : versions | boost::adaptors::reversed) {
      if(ver->name() <= ticket.previous.version)
        break;
      else if(ver->name() <= ticket.version->name())
        m_stream << *ver;
    }
  }
}

void Report::printErrors()
{
  printHeader(L"Errors");

  const auto start = m_stream.tellp();

  for(const ErrorInfo &err : m_receipt.errors()) {
    if(m_stream.tellp() != start)
      m_stream << L"\r\n";

    m_stream << err.context << L":\r\n";
    m_stream.indented(err.message);
  }
}

void Report::printRemovals()
{
  printHeader(L"Removed files");

  for(const Path &path : m_receipt.removals())
    m_stream << path.join() << L"\r\n";
}
