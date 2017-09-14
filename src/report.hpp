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

#ifndef REAPACK_REPORT_HPP
#define REAPACK_REPORT_HPP

#include "dialog.hpp"

class Receipt;
class ReceiptPage;
class TabBar;

class Report : public Dialog {
public:
  Report(const Receipt *);

protected:
  void onInit() override;
  void onTimer(int) override;

private:
  void addPage(const ReceiptPage &);

  const Receipt *m_receipt;
  bool m_empty;
  TabBar *m_tabbar;
  std::vector<std::string> m_pages;
};

#endif
