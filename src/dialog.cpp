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
  RECT dialogRect, rect;

  GetWindowRect(m_handle, &dialogRect);
  GetWindowRect(m_parent, &rect);

  OffsetRect(&dialogRect, -dialogRect.left, -dialogRect.top);
  OffsetRect(&rect, -(dialogRect.right / 2), 0);
  OffsetRect(&rect, 0, -(dialogRect.bottom / 2));

  SetWindowPos(m_handle, HWND_TOP, rect.right / 2, rect.bottom / 1.4,
    0, 0, SWP_NOSIZE);
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
