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
#include <sstream>

using namespace std;

static const string SEP(10, '=');
static const char *NL = "\r\n";

Report::Report(Transaction *transaction)
  : Dialog(IDD_REPORT_DIALOG), m_transaction(transaction)
{
}

void Report::onInit()
{
  const size_t newPacks = m_transaction->newPackages().size();
  const size_t updates = m_transaction->updates().size();
  const size_t errors = m_transaction->errors().size();

  ostringstream text; 

  text
    << newPacks << " new packages, "
    << updates << " updates and "
    << errors << " errors"
    << NL
  ;

  if(errors)
    formatErrors(text);

  if(newPacks)
    formatNewPackages(text);

  if(updates)
    formatUpdates(text);

  const auto_string &str = make_autostring(text.str());
  SetDlgItemText(handle(), IDC_REPORT, str.c_str());
}

void Report::onCommand(WPARAM wParam, LPARAM)
{
  switch(LOWORD(wParam)) {
  case IDOK:
  case IDCANCEL:
    close();
    break;
  }
}

void Report::formatNewPackages(ostringstream &text)
{
  text << NL << SEP << " New packages: " << SEP << NL;

  for(const Transaction::PackageEntry &entry : m_transaction->newPackages()) {
    Version *ver = entry.first;
    text << NL << ver->fullName() << NL;
  }
}

void Report::formatUpdates(ostringstream &text)
{
  text << NL << SEP << " Updates: " << SEP << NL;

  for(const Transaction::PackageEntry &entry : m_transaction->updates()) {
    Package *pkg = entry.first->package();
    const Registry::QueryResult &regEntry = entry.second;
    const VersionSet &versions = pkg->versions();

    for(Version *ver : versions | boost::adaptors::reversed) {
      if(ver->code() <= regEntry.version)
        break;

      text << NL << ver->fullName() << NL;

      if(!ver->changelog().empty())
        formatChangelog(ver->changelog(), text);
    }
  }
}

void Report::formatChangelog(const string &changelog, ostringstream &output)
{
  istringstream input(changelog);
  string line;

  while(getline(input, line, '\n'))
    output << "  " << line.substr(line.find_first_not_of('\x20')) << NL;
}

void Report::formatErrors(ostringstream &text)
{
  text << NL << SEP << " Errors: " << SEP << NL;

  for(const Transaction::Error &err : m_transaction->errors())
    text << NL << err.title << ":" << NL << err.message << NL;
}
