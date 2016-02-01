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

#include <Cocoa/Cocoa.h>

using namespace std;

bool RichEdit::setRichText(const string &rtf)
{
  NSString *str = [NSString
    stringWithCString:rtf.c_str()
    encoding:NSUTF8StringEncoding
  ];

  NSTextView *textView = (NSTextView *)handle();

  [textView
    replaceCharactersInRange: NSMakeRange(0, [[textView string] length])
    withRTF: [str dataUsingEncoding: NSUTF8StringEncoding]
  ];

  const BOOL isEditable = textView.isEditable;

  // auto-detect links, equivalent to Windows' EM_AUTOURLDETECT message
  [textView setEditable:YES];
  [textView setEnabledTextCheckingTypes:NSTextCheckingTypeLink];
  [textView checkTextInDocument:nil];
  [textView setEditable:isEditable];

  // hack: restore NSTextView's default mouse cursors (eg. hover links)
  // this doesn't fix the shy link tooltips
  SetCapture(handle());

  return [[textView string] length];
}
