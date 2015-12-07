#include "report.hpp"

#include "resource.hpp"
#include "transaction.hpp"

#include <sstream>

using namespace std;

static const string SEP(10, '=');

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
    << "\n"
  ;

  if(errors)
    formatErrors(text);

  if(newPacks)
    formatErrors(text);

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
  text << "\n" << SEP << " New packages: " << SEP << "\n";

  for(Package *pkg : m_transaction->newPackages())
    text << "\n- " << pkg->lastVersion()->fullName() << "\n";
}

void Report::formatUpdates(ostringstream &text)
{
  text << "\n" << SEP << " Updates: " << SEP << "\n";

  for(Package *pkg : m_transaction->updates()) {
    Version *ver = pkg->lastVersion();
    text << "\n- " << ver->fullName() << "\n";
    
    if(!ver->changelog().empty())
      text << ver->changelog() << "\n";
  }
}

void Report::formatErrors(ostringstream &text)
{
  text << "\n" << SEP << " Errors: " << SEP << "\n";

  for(const string &msg : m_transaction->errors())
    text << "\n- " << msg << "\n";
}
