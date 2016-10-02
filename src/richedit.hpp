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

#ifndef REAPACK_RICHEDIT_HPP
#define REAPACK_RICHEDIT_HPP

#include "control.hpp"

#include <string>

class RichEdit : public Control {
public:
  static void Init();

  RichEdit(HWND);
  ~RichEdit();

  bool setRichText(const std::string &, bool pretty = true);
  std::string toPlainText() const;
  unsigned long length() const;

protected:
  void onNotify(LPNMHDR, LPARAM) override;
};

#endif
