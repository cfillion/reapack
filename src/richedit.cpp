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

#include "richedit.hpp"

#include <memory>

#include "encoding.hpp"

#ifdef _WIN32
#include <richedit.h>
#endif

using namespace std;

void RichEdit::Init()
{
#ifdef _WIN32
  LoadLibrary(AUTO_STR("Msftedit.dll"));
#endif
}

RichEdit::RichEdit(HWND handle)
  : Control(handle)
{
#ifdef _WIN32
  SendMessage(handle, EM_AUTOURLDETECT, true, 0);
  SendMessage(handle, EM_SETEVENTMASK, 0, ENM_LINK);
#endif
}

void RichEdit::onNotify(LPNMHDR info, LPARAM lParam)
{
#ifdef _WIN32
  switch(info->code) {
  case EN_LINK:
    handleLink(lParam);
    break;
  };
#endif
}

#ifdef _WIN32
void RichEdit::handleLink(LPARAM lParam)
{
  ENLINK *info = (ENLINK *)lParam;
  const CHARRANGE &range = info->chrg;

  auto_char *url = new auto_char[(range.cpMax - range.cpMin) + 1]();
  unique_ptr<auto_char[]> ptr(url);

  TEXTRANGE tr{range, url};
  SendMessage(handle(), EM_GETTEXTRANGE, 0, (LPARAM)&tr);

  if(info->msg == WM_LBUTTONUP)
    ShellExecute(nullptr, AUTO_STR("open"), url, nullptr, nullptr, SW_SHOW);
}

// OS X implementation of setRichText in richedit.mm
void RichEdit::setRichText(const string &rtf)
{
  SETTEXTEX st{};
  SendMessage(handle(), EM_SETTEXTEX, (WPARAM)&st, (LPARAM)rtf.c_str());

  GETTEXTLENGTHEX tl{};
  LONG length = (LONG)SendMessage(handle(), EM_GETTEXTLENGTHEX, (WPARAM)&tl, 0);

  CHARRANGE cr{length, length};
  SendMessage(handle(), EM_EXSETSEL, 0, (LPARAM)&cr);
}
#endif
