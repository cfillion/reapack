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

#include "dialog.hpp"

#include "control.hpp"
#include "encoding.hpp"

#include <algorithm>
#include <boost/range/adaptor/map.hpp>
#include <boost/algorithm/string/join.hpp>
#include <reaper_plugin_functions.h>

using namespace std;

DialogMap Dialog::s_instances;

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

    if(wParam)
      dlg->onShow();
    else
      dlg->onHide();
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
    dlg->onContextMenu((HWND)wParam, LOWORD(lParam), HIWORD(lParam));
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
  : m_template(templateId), m_isVisible(false),
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
  if(dlg->isVisible())
    dlg->onHide();

  dlg->onClose();

  delete dlg;
}

void Dialog::DestroyAll()
{
  const DialogMap map = s_instances; // make an immutable copy
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
    m_closeHandler(result);
    break;
  }
}

void Dialog::center()
{
  RECT dialogRect, parentRect;

  GetWindowRect(m_handle, &dialogRect);
  GetWindowRect(m_parent, &parentRect);

  const int screenWidth = GetSystemMetrics(SM_CXSCREEN);
  const int parentWidth = parentRect.right - parentRect.left;
  const int dialogWidth = dialogRect.right - dialogRect.left;

  int left = (parentWidth - dialogWidth) / 2;
  left = min(left + (int)parentRect.left, screenWidth - dialogWidth);

  const int screenHeight = GetSystemMetrics(SM_CYSCREEN);
  const int parentHeight = parentRect.bottom - parentRect.top;
  const int dialogHeight = dialogRect.bottom - dialogRect.top;

  int top = (parentHeight - dialogHeight) / 2;
  top = min(top + (int)parentRect.top, screenHeight - dialogHeight);

  const int verticalBias = (int)(parentHeight * 0.1);

#ifdef _WIN32
  top -= verticalBias;
#else
  top += verticalBias; // according to SWELL, top means bottom.
#endif

  SetWindowPos(m_handle, HWND_TOP, max(0, left), max(0, top), 0, 0, SWP_NOSIZE);
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

int Dialog::startTimer(const int ms, int id)
{
  if(id == 0) {
    if(m_timers.empty())
      id = 1;
    else
      id = *m_timers.rbegin();
  }

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
  const size_t length = (data.size() * sizeof(auto_char)) + 1; // null terminator

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

void Dialog::onInit()
{
}

void Dialog::onShow()
{
  center();
}

void Dialog::onHide()
{
}

void Dialog::onTimer(int)
{
}

void Dialog::onCommand(const int id, int)
{
  switch(id) {
  case IDOK:
  case IDCANCEL:
    close(id);
  }
}

void Dialog::onNotify(LPNMHDR info, LPARAM lParam)
{
  const auto it = m_controls.find((int)info->idFrom);

  if(it != m_controls.end())
    it->second->onNotify(info, lParam);
}

void Dialog::onContextMenu(HWND, int x, int y)
{
  for(const auto &pair : m_controls) {
    Control *ctrl = pair.second;

    RECT rect;
    GetWindowRect(ctrl->handle(), &rect);

#ifndef _WIN32
    // special treatment for SWELL
    swap(rect.top, rect.bottom);
#endif

    if(y >= rect.top && y <= rect.bottom && x >= rect.left && x <= rect.right) {
      if(ctrl->onContextMenu(m_handle, x, y))
        return;
    }
  }
}

bool Dialog::onKeyDown(int, int)
{
  return false;
}

void Dialog::onClose()
{
}
