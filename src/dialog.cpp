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

#include <algorithm>

#include "control.hpp"

using namespace std;

DialogMap Dialog::s_instances;

WDL_DLGRET Dialog::Proc(HWND handle, UINT msg, WPARAM wParam, LPARAM lParam)
{
  Dialog *dlg = nullptr;

  const auto it = Dialog::s_instances.find(handle);
  if(it != Dialog::s_instances.end())
    dlg = it->second;

  switch(msg) {
  case WM_INITDIALOG:
    dlg = (Dialog *)lParam;
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
#ifdef _WIN32
  case WM_ENABLE: // not supported by SWELL
    dlg->m_isEnabled = wParam == 1;
    break;
#endif
  case WM_TIMER:
    dlg->onTimer();
    break;
  case WM_COMMAND:
    dlg->onCommand(LOWORD(wParam));
    break;
  case WM_NOTIFY:
    dlg->onNotify((LPNMHDR)lParam, lParam);
    break;
  case WM_CONTEXTMENU:
    dlg->onContextMenu((HWND)wParam, LOWORD(lParam), HIWORD(lParam));
    break;
  case WM_DESTROY:
    // On Windows, WM_DESTROY is emitted in place of WM_INITDIALOG
    // if the dialog resource is invalid (eg. because of an unloaded dll).
    //
    // When this happens, neither lParam nor s_instances will contain
    // a pointer to the Dialog instance, so there is nothing we can do.
    // At least, let's try to not crash.
    if(dlg)
      dlg->onDestroy();
    break;
  };

  return false;
}

Dialog::Dialog(const int templateId)
  : m_template(templateId), m_isVisible(false), m_isEnabled(true),
    m_instance(nullptr), m_parent(nullptr), m_handle(nullptr)
{
  // can't call reimplemented virtual methods here during object construction
}

Dialog::~Dialog()
{
  for(const auto &controlPair : m_controls)
    delete controlPair.second;

  DestroyWindow(m_handle);
  s_instances.erase(m_handle);
}

INT_PTR Dialog::init(REAPER_PLUGIN_HINSTANCE inst, HWND parent, Modality mode)
{
  m_instance = inst;
  m_parent = parent;

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

  delete dlg;
}

void Dialog::show()
{
  ShowWindow(m_handle, SW_SHOW);
}

void Dialog::hide()
{
  ShowWindow(m_handle, SW_HIDE);
}

void Dialog::close(const INT_PTR result)
{
  EndDialog(m_handle, result);
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

  SetWindowPos(m_handle, HWND_TOP, max(0, left), max(0, top), 0, 0, SWP_NOSIZE);
}

void Dialog::setFocus()
{
  SetFocus(m_handle);
}

void Dialog::setEnabled(const bool enabled, HWND handle)
{
  EnableWindow(handle, enabled);
}

HWND Dialog::getControl(const int idc)
{
  return GetDlgItem(m_handle, idc);
}

void Dialog::onShow()
{
  center();
}

void Dialog::onHide()
{
}

void Dialog::onTimer()
{
}

void Dialog::onCommand(int)
{
}

void Dialog::onNotify(LPNMHDR info, LPARAM lParam)
{
  const auto it = m_controls.find((int)info->idFrom);

  if(it != m_controls.end())
    it->second->onNotify(info, lParam);
}

void Dialog::onContextMenu(HWND, int, int)
{
}

void Dialog::onDestroy()
{
}
