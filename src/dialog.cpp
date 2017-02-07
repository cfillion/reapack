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

#include "dialog.hpp"

#include "control.hpp"
#include "encoding.hpp"

#include <algorithm>
#include <boost/range/adaptor/map.hpp>
#include <boost/algorithm/string/join.hpp>
#include <reaper_plugin_functions.h>

#ifdef _WIN32
#include <windowsx.h>
#endif

using namespace std;

std::map<HWND, Dialog *> Dialog::s_instances;

WDL_DLGRET Dialog::Proc(HWND handle, UINT msg, WPARAM wParam, LPARAM lParam)
{
  Dialog *dlg;

  if(msg == WM_INITDIALOG)
    dlg = reinterpret_cast<Dialog *>(lParam);
  else {
    const auto it = Dialog::s_instances.find(handle);
    if(it == Dialog::s_instances.end())
      return false;
    else
      dlg = it->second;
  }

  switch(msg) {
  case WM_INITDIALOG:
    dlg->m_handle = handle;

    // necessary if something in onInit triggers another event
    // otherwhise dlg would be null then
    Dialog::s_instances[handle] = dlg;

    dlg->onInit();
    break;
  case WM_SHOWWINDOW:
    // this makes possible to call onHide when destroying the window
    // but only if was visible before the destruction request
    // (destruction might be caused by the application exiting,
    // in which case IsWindowVisible would be false but m_isVisible == true)
    dlg->m_isVisible = wParam == 1;
    break;
  case WM_TIMER:
    dlg->onTimer((int)wParam);
    break;
  case WM_COMMAND:
    dlg->onCommand(LOWORD(wParam), HIWORD(wParam));
    break;
  case WM_NOTIFY:
    dlg->onNotify((LPNMHDR)lParam, lParam);
    break;
  case WM_CONTEXTMENU:
    dlg->onContextMenu((HWND)wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    break;
  case WM_GETMINMAXINFO: {
    MINMAXINFO *mmi = (MINMAXINFO *)lParam;
    mmi->ptMinTrackSize.x = dlg->m_minimumSize.x;
    mmi->ptMinTrackSize.y = dlg->m_minimumSize.y;
    break;
  }
  case WM_SIZE:
    if(wParam != SIZE_MINIMIZED)
      dlg->onResize();
    break;
  case WM_DESTROY:
    // On Windows, WM_DESTROY is emitted in place of WM_INITDIALOG
    // if the dialog resource is invalid (eg. because of an unloaded dll).
    //
    // When this happens, neither lParam nor s_instances will contain
    // a pointer to the Dialog instance, so there is nothing we can do.
    // At least, let's try to not crash.
    if(dlg)
      dlg->onClose();
    break;
  };

  return false;
}

int Dialog::HandleKey(MSG *msg, accelerator_register_t *accel)
{
  Dialog *dialog = reinterpret_cast<Dialog *>(accel->user);
  if(!dialog || !dialog->hasFocus())
    return 0; // not our window

  const int key = (int)msg->wParam;
  int modifiers = 0;

  if(GetAsyncKeyState(VK_MENU) & 0x8000)
    modifiers |= AltModifier;
  if(GetAsyncKeyState(VK_CONTROL) & 0x8000)
    modifiers |= CtrlModifier;
  if(GetAsyncKeyState(VK_SHIFT) & 0x8000)
    modifiers |= ShiftModifier;

  if(msg->message == WM_KEYDOWN && dialog->onKeyDown(key, modifiers))
    return 1;
  else
    return -1;
}

Dialog::Dialog(const int templateId)
  : m_template(templateId), m_isVisible(false), m_minimumSize(),
    m_instance(nullptr), m_parent(nullptr), m_handle(nullptr)
{
  m_accel.translateAccel = HandleKey;
  m_accel.isLocal = true;
  m_accel.user = this;
  plugin_register("accelerator", &m_accel);

  // don't call reimplemented virtual methods here during object construction
}

Dialog::~Dialog()
{
  plugin_register("-accelerator", &m_accel);

  const set<int> timers = m_timers; // make an immutable copy
  for(const int id : timers)
    stopTimer(id);

  for(Control *control : m_controls | boost::adaptors::map_values)
    delete control;

  // Unregistering the instance right now before DestroyWindow prevents WM_DESTROY
  // from calling the default implementation of onClose (because we're in the
  // destructor – no polymorphism here). Instead, the right onClose is called
  // directly by close() for modeless dialogs, or by the OS/SWELL for modal dialogs.
  s_instances.erase(m_handle);

  DestroyWindow(m_handle);
}

INT_PTR Dialog::init(REAPER_PLUGIN_HINSTANCE inst, HWND parent, Modality mode)
{
  m_instance = inst;
  m_parent = parent;
  m_mode = mode;

  switch(mode) {
  case Modeless:
    CreateDialogParam(inst, MAKEINTRESOURCE(m_template),
      m_parent, Proc, (LPARAM)this);
    return true;
  case Modal:
    return DialogBoxParam(inst, MAKEINTRESOURCE(m_template),
      m_parent, Proc, (LPARAM)this);
  }

  return false; // makes MSVC happy.
}

void Dialog::Destroy(Dialog *dlg)
{
  delete dlg;
}

void Dialog::DestroyAll()
{
  const auto map = s_instances; // make an immutable copy
  for(Dialog *dlg : map | boost::adaptors::map_values)
    Destroy(dlg);
}

void Dialog::setVisible(const bool visible, HWND handle)
{
  ShowWindow(handle, visible ? SW_SHOW : SW_HIDE);
}

void Dialog::close(const INT_PTR result)
{
  switch(m_mode) {
  case Modal:
    EndDialog(m_handle, result);
    break;
  case Modeless:
    onClose();
    m_closeHandler(result);
    break;
  }
}

void Dialog::center()
{
  RECT dialogRect, parentRect;

  GetWindowRect(m_handle, &dialogRect);
  GetWindowRect(m_parent, &parentRect);

#ifdef _WIN32
  HMONITOR monitor = MonitorFromWindow(m_parent, MONITOR_DEFAULTTONEAREST);
  MONITORINFO minfo{sizeof(MONITORINFO)};
  GetMonitorInfo(monitor, &minfo);
  RECT &screenRect = minfo.rcWork;
#else
  RECT screenRect;
  SWELL_GetViewPort(&screenRect, &dialogRect, false);
#endif

  // limit the centering to the monitor containing most of the parent window
  parentRect.left = max(parentRect.left, screenRect.left);
  parentRect.top = max(parentRect.top, screenRect.top);
  parentRect.right = min(parentRect.right, screenRect.right);
  parentRect.bottom = min(parentRect.bottom, screenRect.bottom);

  const int parentWidth = parentRect.right - parentRect.left;
  const int dialogWidth = dialogRect.right - dialogRect.left;
  int left = (parentWidth - dialogWidth) / 2;
  left += parentRect.left;

  const int parentHeight = parentRect.bottom - parentRect.top;
  const int dialogHeight = dialogRect.bottom - dialogRect.top;
  int top = (parentHeight - dialogHeight) / 2;
  top += parentRect.top;

  const double verticalBias = (top - parentRect.top) * 0.3;

#ifdef _WIN32
  top -= (int)verticalBias;
#else
  top += verticalBias; // according to SWELL, top means bottom.
#endif

  boundedMove(left, top);
}

void Dialog::boundedMove(int x, int y)
{
  RECT rect;
  GetWindowRect(m_handle, &rect);

  const int width = rect.right - rect.left, height = rect.bottom - rect.top;

#ifdef SM_XVIRTUALSCREEN
  // virtual screen = all screens combined
  const int viewportX = GetSystemMetrics(SM_XVIRTUALSCREEN);
  const int viewportWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
  const int viewportY = GetSystemMetrics(SM_YVIRTUALSCREEN);
  const int viewportHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
#else
  // SWELL_GetViewPort only gives the rect of the current screen
  RECT viewport;
  SWELL_GetViewPort(&viewport, &rect, false);

  const int viewportX = viewport.left;
  const int viewportWidth = viewport.right - viewportX;
  const int viewportY = viewport.top;
  const int viewportHeight = viewport.bottom - viewportY;
#endif

  x = min(max(viewportX, x), viewportWidth - width - abs(viewportX));
  y = min(max(viewportY, y), viewportHeight - height - abs(viewportY));

  SetWindowPos(m_handle, nullptr, x, y,
    0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
}

bool Dialog::hasFocus() const
{
  const HWND focused = GetFocus();
  return focused == m_handle || IsChild(m_handle, focused);
}

void Dialog::setFocus()
{
  show(); // hack to unminimize the window on OS X
  SetFocus(m_handle);
}

void Dialog::setEnabled(const bool enabled, HWND handle)
{
  EnableWindow(handle, enabled);
}

int Dialog::startTimer(const int ms, int id, const bool replace)
{
  if(id == 0) {
    if(m_timers.empty())
      id = 1;
    else
      id = *m_timers.rbegin() + 1;
  }
  else if(!replace && m_timers.count(id))
    return 0;

  m_timers.insert(id);
  SetTimer(m_handle, id, ms, nullptr);

  return id;
}

void Dialog::stopTimer(int id)
{
  KillTimer(m_handle, id);
  m_timers.erase(id);
}

void Dialog::setClipboard(const string &text)
{
  const auto_string &data = make_autostring(text);
  const size_t length = (data.size() + 1) * sizeof(auto_char); // null terminator

  HANDLE mem = GlobalAlloc(GMEM_MOVEABLE, length);
  memcpy(GlobalLock(mem), data.c_str(), length);
  GlobalUnlock(mem);

  OpenClipboard(m_handle);
  EmptyClipboard();
#ifdef _WIN32
  SetClipboardData(CF_UNICODETEXT, mem);
#else
  SetClipboardData(CF_TEXT, mem);
#endif
  CloseClipboard();
}

void Dialog::setClipboard(const vector<string> &values)
{
  if(!values.empty())
    setClipboard(boost::algorithm::join(values, "\n"));
}

HWND Dialog::getControl(const int idc)
{
  return GetDlgItem(m_handle, idc);
}

string Dialog::getText(HWND handle)
{
  auto_string buffer(4096, 0);
  GetWindowText(handle, &buffer[0], (int)buffer.size());

  // remove extra nulls from the string
  buffer.resize(buffer.find(AUTO_STR('\0')));

  return from_autostring(buffer);
}

void Dialog::setAnchor(HWND handle, const int flags)
{
  const float left = (float)min(1, flags & AnchorLeft);
  const float top = (float)min(1, flags & AnchorTop);
  const float right = (float)min(1, flags & AnchorRight);
  const float bottom = (float)min(1, flags & AnchorBottom);

  m_resizer.remove_itemhwnd(handle);
  m_resizer.init_itemhwnd(handle, left, top, right, bottom);
}

void Dialog::restoreState(Serializer::Data &data)
{
  if(data.size() < 2)
    return;

  auto it = data.begin();

  const auto &pos = *it++;
  const auto &size = *it++;

  SetWindowPos(m_handle, nullptr, 0, 0,
    size[0], size[1], SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
  onResize();

  boundedMove(pos[0], pos[1]);

  data.erase(data.begin(), it);
}

void Dialog::saveState(Serializer::Data &data) const
{
  RECT rect;
  GetWindowRect(m_handle, &rect);

  data.push_back({rect.left, rect.top});
  data.push_back({rect.right - rect.left, rect.bottom - rect.top});
}

void Dialog::onInit()
{
  RECT rect;
  GetWindowRect(m_handle, &rect);
  m_minimumSize = {rect.right - rect.left, rect.bottom - rect.top};

  center();

  m_resizer.init(m_handle);
}

void Dialog::onTimer(int)
{
}

void Dialog::onCommand(const int id, int)
{
  switch(id) {
  case IDOK:
    close(1);
    break;
  case IDCANCEL:
    close(0);
    break;
  }
}

void Dialog::onNotify(LPNMHDR info, LPARAM lParam)
{
  const auto it = m_controls.find((int)info->idFrom);

  if(it != m_controls.end())
    it->second->onNotify(info, lParam);
}

void Dialog::onContextMenu(HWND target, const int x, const int y)
{
  for(const auto &pair : m_controls) {
    Control *ctrl = pair.second;

    if(!IsWindowVisible(ctrl->handle()))
      continue;

    // target HWND is not always accurate:
    // on OS X it does not match the listview when hovering the column header

    RECT rect;
    GetWindowRect(ctrl->handle(), &rect);

#ifndef _WIN32
    // special treatment for SWELL
    swap(rect.top, rect.bottom);
#endif

    const POINT point{x, y};
    if(target == ctrl->handle() || PtInRect(&rect, point)) {
      if(ctrl->onContextMenu(m_handle, x, y))
        return;
    }
  }
}

bool Dialog::onKeyDown(int, int)
{
  return false;
}

void Dialog::onResize()
{
  m_resizer.onResize();
}

void Dialog::onClose()
{
}
