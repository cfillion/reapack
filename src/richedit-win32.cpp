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

#include "richedit.hpp"

#ifdef _WIN32

// Starting here and onward is the Win32 implementation of RichEdit
// The OS X implementation can be found in richedit.mm

#include "encoding.hpp"

#include <memory>
#include <richedit.h>
#include <sstream>

using namespace std;

static void HandleLink(ENLINK *info, HWND handle)
{
  const CHARRANGE &range = info->chrg;

  auto_char *url = new auto_char[(range.cpMax - range.cpMin) + 1]();
  unique_ptr<auto_char[]> ptr(url);

  TEXTRANGE tr{range, url};
  SendMessage(handle, EM_GETTEXTRANGE, 0, (LPARAM)&tr);

  if(info->msg == WM_LBUTTONUP)
    ShellExecute(nullptr, AUTO_STR("open"), url, nullptr, nullptr, SW_SHOW);
}

void RichEdit::Init()
{
  LoadLibrary(AUTO_STR("Msftedit.dll"));
}

RichEdit::RichEdit(HWND handle)
  : Control(handle)
{
  SendMessage(handle, EM_AUTOURLDETECT, true, 0);
  SendMessage(handle, EM_SETEVENTMASK, 0, ENM_LINK);
  SendMessage(handle, EM_SETEDITSTYLE,
    SES_HYPERLINKTOOLTIPS, SES_HYPERLINKTOOLTIPS);
}

RichEdit::~RichEdit()
{
}

void RichEdit::onNotify(LPNMHDR info, LPARAM lParam)
{
  switch(info->code) {
  case EN_LINK:
    HandleLink((ENLINK *)lParam, handle());
    break;
  };
}

bool RichEdit::setRichText(const string &rtf)
{
  stringstream stream(rtf);

  EDITSTREAM es{};
  es.dwCookie = (DWORD_PTR)&stream;
  es.pfnCallback = [](DWORD_PTR cookie, LPBYTE buf, LONG size, LONG *pcb)
  {
    stringstream *stream = reinterpret_cast<stringstream *>(cookie);
    *pcb = (LONG)stream->readsome((char *)buf, size);
    return (DWORD)0;
  };

  SendMessage(handle(), EM_STREAMIN, SF_RTF, (LPARAM)&es);

  if(es.dwError)
    return false;

  GETTEXTLENGTHEX tl{};
  LONG length = (LONG)SendMessage(handle(), EM_GETTEXTLENGTHEX, (WPARAM)&tl, 0);

  if(!length)
    return false;

  // scale down a little bit, by default everything is way to big
  SendMessage(handle(), EM_SETZOOM, 3, 4);

  return true;
}
#endif
