#ifndef REAPACK_PROGRESS_HPP
#define REAPACK_PROGRESS_HPP

#include "dialog.hpp"

class Download;
class Transaction;

class Progress : public Dialog {
public:
  Progress();

  void setTransaction(Transaction *t);

protected:
  void onInit() override;
  void onCommand(WPARAM, LPARAM) override;

private:
  void addDownload(Download *);
  void updateProgress();

  Transaction *m_transaction;
  Download *m_current;

  HWND m_label;
  HWND m_progress;

  int m_done;
  int m_total;
};

#endif
