#ifndef REAPACK_REPORT_HPP
#define REAPACK_REPORT_HPP

#include "dialog.hpp"

class Transaction;

class Report : public Dialog {
public:
  Report(Transaction *);

protected:
  void onInit() override;
  void onCommand(WPARAM, LPARAM) override;

private:
  void formatNewPackages(std::ostringstream &);
  void formatUpdates(std::ostringstream &);
  void formatErrors(std::ostringstream &);

  Transaction *m_transaction;
};

#endif
