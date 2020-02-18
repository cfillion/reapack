#include "dialog.hpp"

#include <AppKit/NSTextView.h>
#include <swell/swell.h>

bool Dialog::isTextEditUnderMouse() const
{
  POINT p;
  GetCursorPos(&p);
  const HWND ctrl = WindowFromPoint(p);

  return ctrl && [static_cast<id>(ctrl) isKindOfClass:[NSTextView class]];
}
