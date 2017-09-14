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

#include "receipt.hpp"
#include "resource.hpp"
#include "tabbar.hpp"
#include "win32.hpp"

using namespace std;

Report::Report(const Receipt *receipt)
  : Dialog(IDD_REPORT_DIALOG), m_receipt(receipt)
{
}

void Report::onInit()
{
  Dialog::onInit();

  HWND report = getControl(IDC_REPORT);
  TabBar *tabbar = createControl<TabBar>(IDC_TABS, this);

  tabbar->onTabChange([=] (const int i) {
    Win32::setWindowText(report, m_pages[i].c_str());
  });

  bool firstPage = true;

  for(const ReceiptPage &page : m_receipt->pages()) {
    m_pages.emplace_back(page.contents());
    tabbar->addTab({page.title().c_str()});

    if(firstPage && !page.empty()) {
      tabbar->setCurrentIndex(tabbar->count() - 1);
      firstPage = false;
    }
  }

  SetFocus(getControl(IDOK));

  if(m_receipt->test(Receipt::RestartNeeded))
    startTimer(1);
}

void Report::onTimer(int timer)
{
  stopTimer(timer);

  Win32::messageBox(handle(),
    "One or more native REAPER extensions were installed.\r\n"
    "These newly installed files won't be loaded until REAPER is restarted.",
    "ReaPack Notice", MB_OK);
}
