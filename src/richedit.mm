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

#include <Cocoa/Cocoa.h>

#include "string.hpp"

void RichEdit::Init()
{
}

RichEdit::RichEdit(HWND handle)
  : Control(handle)
{
  // hack: restore NSTextView's default mouse cursors (eg. hover links)
  // this is an incomplete fix for the hyperlink's shy tooltips
  SetCapture(handle);
}

RichEdit::~RichEdit()
{
  if(GetCapture() == handle())
    ReleaseCapture();
}

void RichEdit::onNotify(LPNMHDR, LPARAM)
{
}

bool RichEdit::setRichText(const String &rtf)
{
  NSString *str = [NSString
    stringWithCString:rtf.c_str()
    encoding:NSUTF8StringEncoding
  ];

  NSTextView *textView = (NSTextView *)handle();

  // Manually clear the view so that invalid content will always result
  // in this function to return false. Without this, the old text would be
  // retained, length would stay the same and we would return true.
  [textView setString: @""];

  [textView
    replaceCharactersInRange: NSMakeRange(0, [[textView string] length])
    withRTF: [str dataUsingEncoding: NSUTF8StringEncoding]
  ];

  // auto-detect links, equivalent to Windows' EM_AUTOURLDETECT message
  const BOOL isEditable = textView.isEditable;
  [textView setEditable: YES];
  [textView setEnabledTextCheckingTypes: NSTextCheckingTypeLink];
  [textView checkTextInDocument: nil];
  [textView setEditable: isEditable];

  return [[textView string] length];
}
