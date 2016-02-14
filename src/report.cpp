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

#include "report.hpp"

#include "encoding.hpp"
#include "package.hpp"
#include "resource.hpp"
#include "transaction.hpp"

#include <boost/range/adaptor/reversed.hpp>
#include <locale>

using namespace std;

static const string SEP(10, '=');
const char * const ReportDialog::NL = "\r\n";

ReportDialog::ReportDialog()
  : Dialog(IDD_REPORT_DIALOG)
{
  // enable number formatting (ie. "1,234" instead of "1234")
  m_stream.imbue(locale(""));
}

void ReportDialog::onInit()
{
  fillReport();

  const auto_string &str = make_autostring(m_stream.str());
  SetDlgItemText(handle(), IDC_REPORT, str.c_str());
}

void ReportDialog::printHeader(const char *title)
{
  if(m_stream.tellp())
    m_stream << NL;

  m_stream << SEP << ' ' << title << ": " << SEP << NL;
}

void ReportDialog::printChangelog(const string &changelog)
{
  istringstream input(changelog);
  string line;

  while(getline(input, line, '\n'))
    m_stream << "\x20\x20" << line.substr(line.find_first_not_of('\x20')) << NL;
}

Report::Report(Transaction *transaction)
  : ReportDialog(), m_transaction(transaction)
{
}

void Report::fillReport()
{
  const size_t newPackages = m_transaction->newPackages().size();
  const size_t updates = m_transaction->updates().size();
  const size_t removals = m_transaction->removals().size();
  const size_t errors = m_transaction->errors().size();

  stream()
    << newPackages << " new packages, "
    << updates << " updates, "
    << removals << " removed files and "
    << errors << " errors"
    << NL
  ;

  if(m_transaction->isRestartNeeded()) {
    stream()
      << NL
      << "Notice: One or more native REAPER extensions were installed." << NL
      << "The newly installed files won't be loaded until REAPER is restarted."
      << NL;
  }

  if(errors)
    printErrors();

  if(newPackages)
    printNewPackages();

  if(updates)
    printUpdates();

  if(removals)
    printRemovals();
}

void Report::printNewPackages()
{
  printHeader("New packages");

  for(const Transaction::InstallTicket &entry : m_transaction->newPackages()) {
    const Version *ver = entry.first;
    stream() << NL << ver->fullName();
  }
}

void Report::printUpdates()
{
  printHeader("Updates");

  for(const Transaction::InstallTicket &entry : m_transaction->updates()) {
    const Package *pkg = entry.first->package();
    const auto &queryRes = entry.second;
    const VersionSet &versions = pkg->versions();

    for(const Version *ver : versions | boost::adaptors::reversed) {
      if(ver->code() <= queryRes.entry.version)
        break;

      stream() << NL << ver->fullName() << NL;

      if(!ver->changelog().empty())
        printChangelog(ver->changelog());
    }
  }
}

void Report::printErrors()
{
  printHeader("Errors");

  for(const Transaction::Error &err : m_transaction->errors())
    stream() << NL << err.title << ':' << NL << err.message << NL;
}

void Report::printRemovals()
{
  printHeader("Removed files");

  for(const Path &path : m_transaction->removals())
    stream() << NL << path.join();
}
