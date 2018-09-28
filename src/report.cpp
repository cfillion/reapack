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

#include "report.hpp"

#include "receipt.hpp"
#include "resource.hpp"
#include "tabbar.hpp"
#include "win32.hpp"

using namespace std;

Report::Report(const Receipt *receipt)
  : Dialog(IDD_REPORT_DIALOG), m_receipt(receipt), m_empty(true)
{
}

void Report::onInit()
{
  Dialog::onInit();

  m_tabbar = createControl<TabBar>(IDC_TABS, this);
  m_tabbar->onTabChange >> [=] (const int i) {
    Win32::setWindowText(getControl(IDC_REPORT), m_pages[i].c_str());
  };

  const ReceiptPage pages[] = {
    m_receipt->installedPage(),
    m_receipt->removedPage(),
    m_receipt->exportedPage(),
    m_receipt->errorPage(),
  };

  for(const auto &page : pages)
    addPage(page);

  updateLabel();

  SetFocus(getControl(IDOK));

  if(m_receipt->test(Receipt::RestartNeededFlag))
    startTimer(1);
}

void Report::onTimer(int timer)
{
  stopTimer(timer);

  Win32::messageBox(handle(),
    "One or more native REAPER extensions were installed.\n"
    "These newly installed files won't be loaded until REAPER is restarted.",
    "ReaPack Notice", MB_OK);
}

void Report::updateLabel()
{
  const char *label;

  if(m_receipt->flags() == Receipt::ErrorFlag)
    label = "Operation failed. The following error(s) occured:";
  else if(m_receipt->test(Receipt::ErrorFlag))
    label = "The operation was partially completed (one or more errors occured):";
  else if(m_receipt->test(Receipt::InstalledOrRemoved))
    label = "All done! Description of the changes:";
  else
    label = "Operation completed successfully!";

  Win32::setWindowText(getControl(IDC_LABEL), label);
}

void Report::addPage(const ReceiptPage &page)
{
  m_pages.emplace_back(page.contents());
  m_tabbar->addTab({page.title().c_str()});

  if(m_empty && !page.empty()) {
    m_tabbar->setCurrentIndex(m_tabbar->count() - 1);
    m_empty = false;
  }
}
