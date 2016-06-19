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
#include "source.hpp"
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

  m_stream << SEP << ' ' << title << ": " << SEP << NL << NL;
}

void ReportDialog::printVersion(const Version *ver)
{
  stream() << 'v' << ver->name();

  if(!ver->author().empty())
    stream() << " by " << ver->author();

  const string &date = ver->displayTime();
  if(!date.empty())
    stream() << " – " << date;

  stream() << NL;
}

void ReportDialog::printChangelog(const Version *ver)
{
  const string &changelog = ver->changelog();
  printIndented(changelog.empty() ? "No changelog" : changelog);
}

void ReportDialog::printIndented(const string &text)
{
  istringstream stream(text);
  string line;

  while(getline(stream, line, '\n'))
    m_stream << "\x20\x20" << line.substr(line.find_first_not_of('\x20')) << NL;
}

Report::Report(const Receipt &receipt)
  : ReportDialog(), m_receipt(receipt)
{
}

void Report::fillReport()
{
  const size_t installs = m_receipt.installs().size();
  const size_t updates = m_receipt.updates().size();
  const size_t removals = m_receipt.removals().size();
  const size_t errors = m_receipt.errors().size();

  stream()
    << installs << " installed packages, "
    << updates << " updates, "
    << removals << " removed files and "
    << errors << " errors"
    << NL
  ;

  if(m_receipt.isRestartNeeded()) {
    stream()
      << NL
      << "Notice: One or more native REAPER extensions were installed." << NL
      << "The newly installed files won't be loaded until REAPER is restarted."
      << NL;
  }

  if(errors)
    printErrors();

  if(installs)
    printInstalls();

  if(updates)
    printUpdates();

  if(removals)
    printRemovals();
}

void Report::printInstalls()
{
  printHeader("Installed packages");

  for(const InstallTicket &ticket : m_receipt.installs())
    stream() << ticket.version->fullName() << NL;
}

void Report::printUpdates()
{
  printHeader("Updates");

  const auto start = stream().tellp();

  for(const InstallTicket &ticket : m_receipt.updates()) {
    const Package *pkg = ticket.version->package();
    const Registry::Entry &regEntry = ticket.regEntry;
    const VersionSet &versions = pkg->versions();

    if(stream().tellp() != start)
      stream() << NL;

    stream() << pkg->fullName() << ':' << NL;

    for(const Version *ver : versions | boost::adaptors::reversed) {
      if(*ver <= regEntry.version)
        break;

      printVersion(ver);
      printChangelog(ver);
    }
  }
}

void Report::printErrors()
{
  printHeader("Errors");

  const auto start = stream().tellp();

  for(const Receipt::Error &err : m_receipt.errors()) {
    if(stream().tellp() != start)
      stream() << NL;

    stream() << err.title << ':' << NL;
    printIndented(err.message);
  }
}

void Report::printRemovals()
{
  printHeader("Removed files");

  for(const Path &path : m_receipt.removals())
    stream() << path.join() << NL;
}

History::History(const Package *pkg)
  : ReportDialog(), m_package(pkg)
{
}

void History::fillReport()
{
  SetWindowText(handle(), AUTO_STR("Package History"));
  SetWindowText(getControl(IDC_LABEL),
    make_autostring(m_package->name()).c_str());

  for(const Version *ver : m_package->versions() | boost::adaptors::reversed) {
    if(stream().tellp())
      stream() << NL;

    printVersion(ver);
    printChangelog(ver);
  }
}

Contents::Contents(const Package *pkg)
  : ReportDialog(), m_package(pkg)
{
}

void Contents::fillReport()
{
  SetWindowText(handle(), AUTO_STR("Package Contents"));
  SetWindowText(getControl(IDC_LABEL),
    make_autostring(m_package->name()).c_str());

  for(const Version *ver : m_package->versions() | boost::adaptors::reversed) {
    if(stream().tellp())
      stream() << NL;

    printVersion(ver);

    const auto &sources = ver->sources();
    for(auto it = sources.begin(); it != sources.end();) {
      const Path &path = it->first;
      const string &file = path.join();
      const Source *src = it->second;

      printIndented(src->isMain() ? file + '*' : file);
      printIndented("Source: " + src->url());

      // skip duplicate files
      do { it++; } while(it != sources.end() && path == it->first);
    }
  }
}
