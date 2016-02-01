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

#include "about.hpp"

#include <Cocoa/Cocoa.h>

void About::setAboutText(const auto_string &text)
{
  NSString *str = [NSString
    stringWithCString:text.c_str()
    encoding:NSUTF8StringEncoding
  ];

  NSTextView *textView = (NSTextView *)m_about;

  [textView
    replaceCharactersInRange: NSMakeRange(0, [[textView string] length])
    withRTF: [str dataUsingEncoding: NSUTF8StringEncoding]
  ];
}
