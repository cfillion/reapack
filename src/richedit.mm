/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2021  Christian Fillion
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

#include <AppKit/NSAttributedString.h>
#include <AppKit/NSColor.h>
#include <AppKit/NSTextView.h>

void RichEdit::Init()
{
}

RichEdit::RichEdit(HWND handle)
  : Control(handle)
{
}

RichEdit::~RichEdit() = default;

void RichEdit::onNotify(LPNMHDR, LPARAM)
{
}

void RichEdit::setPlainText(const std::string &text)
{
  NSString *str = [NSString
    stringWithCString: text.c_str()
    encoding: NSUTF8StringEncoding
  ];

  NSDictionary *darkMode = [NSDictionary
    dictionaryWithObject: NSColor.textColor
                  forKey: NSForegroundColorAttributeName];

  NSAttributedString *attrStr = [[NSAttributedString alloc]
   initWithString: str attributes: darkMode];

  auto textView = static_cast<NSTextView *>(handle());
  [[textView textStorage] setAttributedString: attrStr];
  [attrStr release];
}

bool RichEdit::setRichText(const std::string &rtf)
{
  NSString *str = [NSString
    stringWithCString: rtf.c_str()
    encoding: NSUTF8StringEncoding
  ];

  NSAttributedString *attrStr = [[NSAttributedString alloc]
    initWithRTF: [str dataUsingEncoding: NSUTF8StringEncoding]
    documentAttributes: nullptr];

  if(!attrStr)
    return false;

  auto textView = static_cast<NSTextView *>(handle());
  [[textView textStorage] setAttributedString: attrStr];
  [textView setTextColor: NSColor.textColor]; // dark mode support
  [attrStr release];

  // auto-detect links, equivalent to Windows' EM_AUTOURLDETECT message
  const BOOL isEditable = textView.isEditable;
  [textView setEditable: YES];
  [textView setEnabledTextCheckingTypes: NSTextCheckingTypeLink];
  [textView checkTextInDocument: nil];
  [textView setEditable: isEditable];

  return [[textView string] length];
}
