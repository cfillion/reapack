#include "dialog.hpp"

DialogMap Dialog::s_instances;

WDL_DLGRET Dialog::Proc(HWND handle, UINT msg, WPARAM wParam, LPARAM lParam)
{
  Dialog *dlg = 0;

  const auto it = Dialog::s_instances.find(handle);
  if(it != Dialog::s_instances.end())
    dlg = it->second;

  switch(msg) {
  case WM_INITDIALOG:
    dlg = (Dialog *)lParam;
    dlg->m_handle = handle;
    Dialog::s_instances[handle] = dlg;

    dlg->onInit();
    break;
  case WM_SHOWWINDOW:
    if(wParam) {
      dlg->m_isVisible = true;
      dlg->onShow();
    }
    else {
      dlg->m_isVisible = false;
      dlg->onHide();
    }
    break;
  case WM_TIMER:
    dlg->onTimer();
    break;
  case WM_COMMAND:
    dlg->onCommand(wParam, lParam);
    break;
  case WM_DESTROY:
    dlg->onDestroy();
    break;
  };

  return false;
}

Dialog::Dialog(const int templateId)
  : m_template(templateId), m_parent(0), m_handle(0), m_isVisible(false)
{
}

void Dialog::init(REAPER_PLUGIN_HINSTANCE instance, HWND parent)
{
  m_parent = parent;

  CreateDialogParam(instance, MAKEINTRESOURCE(m_template),
    m_parent, Proc, (LPARAM)this);
}

void Dialog::Destroy(Dialog *dlg)
{
  if(dlg->isVisible())
    dlg->onHide();

  delete dlg;
}

Dialog::~Dialog()
{
  DestroyWindow(m_handle);
  s_instances.erase(m_handle);
}

void Dialog::show()
{
  center();

  ShowWindow(m_handle, SW_SHOW);
}

void Dialog::hide()
{
  ShowWindow(m_handle, SW_HIDE);
}

void Dialog::center()
{
  RECT dialogRect, parentRect;

  GetWindowRect(m_handle, &dialogRect);
  GetWindowRect(m_parent, &parentRect);

  int left = parentRect.right / 2;
  left -= (dialogRect.right - dialogRect.left) / 2;

  int top = parentRect.bottom / 2;
  top -= (dialogRect.bottom - dialogRect.top) / 2;

  SetWindowPos(m_handle, HWND_TOP, left, top, 0, 0, SWP_NOSIZE);
}

void Dialog::onInit()
{
}

void Dialog::onShow()
{
}

void Dialog::onHide()
{
}

void Dialog::onTimer()
{
}

void Dialog::onCommand(WPARAM, LPARAM)
{
}

void Dialog::onDestroy()
{
}
