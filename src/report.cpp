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

  for(Package *pkg : m_transaction->newPackages())
    text << NL << "- " << pkg->lastVersion()->fullName() << NL;
}

void Report::formatUpdates(ostringstream &text)
{
  text << NL << SEP << " Updates: " << SEP << NL;

  for(Package *pkg : m_transaction->updates()) {
    Version *ver = pkg->lastVersion();
    text << NL << "- " << ver->fullName() << NL;
    
    if(!ver->changelog().empty())
      text << ver->changelog() << NL;
  }
}

void Report::formatErrors(ostringstream &text)
{
  text << NL << SEP << " Errors: " << SEP << NL;

  for(const string &msg : m_transaction->errors())
    text << NL << "- " << msg << NL;
}
