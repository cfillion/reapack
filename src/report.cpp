#include "report.hpp"

#include "resource.hpp"
#include "transaction.hpp"

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

  SetDlgItemText(handle(), IDC_REPORT, text.str().c_str());
}

void Report::onCommand(WPARAM wParam, LPARAM)
{
  const int commandId = LOWORD(wParam);

  switch(commandId) {
  case IDOK:
  case IDCANCEL:
    EndDialog(handle(), true);
    break;
  }
}

void Report::formatNewPackages(ostringstream &text)
{
  text << NL << SEP << " New packages: " << SEP << NL;

  for(const Transaction::PackageEntry &entry : m_transaction->newPackages())
    text << NL << "- " << entry.first->lastVersion()->fullName() << NL;
}

void Report::formatUpdates(ostringstream &text)
{
  text << NL << SEP << " Updates: " << SEP << NL;

  for(const Transaction::PackageEntry &entry : m_transaction->updates()) {
    Package *pkg = entry.first;
    const Registry::QueryResult &regEntry = entry.second;
    const VersionSet &versions = pkg->versions();

    for(auto it = versions.rbegin(); it != versions.rend(); it++) {
      Version *ver = *it;

      if(ver->code() <= regEntry.versionCode)
        break;

      text << NL << "- " << ver->fullName() << NL;

      if(!ver->changelog().empty())
        text << ver->changelog() << NL;
    }
  }
}

void Report::formatErrors(ostringstream &text)
{
  text << NL << SEP << " Errors: " << SEP << NL;

  for(const Transaction::Error &err : m_transaction->errors())
    text << NL << err.title << ":" << NL << err.message << NL;
}
