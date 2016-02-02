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

#ifndef REAPACK_DIALOG_HPP
#define REAPACK_DIALOG_HPP

#include <map>

#include <wdltypes.h>
#include <reaper_plugin.h>

class Dialog;
typedef std::map<HWND, Dialog *> DialogMap;

class Control;

class Dialog {
public:
  enum Modality {
    Modeless,
    Modal,
  };

  template<class T, class... Args>
  static T *Create(REAPER_PLUGIN_HINSTANCE instance, HWND parent, Args&&... args)
  {
    Dialog *dlg = new T(args...);
    dlg->init(instance, parent, Dialog::Modeless);

    return dynamic_cast<T *>(dlg);
  }

  template<class T, class... Args>
  static INT_PTR Show(REAPER_PLUGIN_HINSTANCE i, HWND parent, Args&&... args)
  {
    Dialog *dlg = new T(args...);
    INT_PTR ret = dlg->init(i, parent, Dialog::Modal);
    Destroy(dlg);

    return ret;
  }

  static void Destroy(Dialog *);

  INT_PTR init(REAPER_PLUGIN_HINSTANCE, HWND, const Modality);

  REAPER_PLUGIN_HINSTANCE instance() const { return m_instance; }
  HWND parent() const { return m_parent; }
  HWND handle() const { return m_handle; }

  bool isVisible() const { return m_isVisible; }
  bool isEnabled() const { return m_isEnabled; }

  void enable() { enable(m_handle); }
  void enable(HWND handle) { setEnabled(true, handle); }
  void disable() { disable(m_handle); }
  void disable(HWND handle) { setEnabled(false, handle); }
  void setEnabled(const bool enable) { setEnabled(enable, m_handle); }
  void setEnabled(const bool, HWND);

  void show(HWND);
  void show() { show(m_handle); }
  void hide(HWND);
  void hide() { hide(m_handle); }
  void close(const INT_PTR = 0);
  void center();
  void setFocus();

protected:
  Dialog(const int templateId);
  virtual ~Dialog();

  HWND getControl(const int idc);

  template<class T, class... Args>
  T *createControl(int id, Args&&... args)
  {
    if(m_controls.count(id))
      return nullptr;

    HWND handle = getControl(id);

    T *ctrl = new T(args..., handle);
    m_controls[id] = ctrl;

    return ctrl;
  }

  virtual void onInit() = 0;
  virtual void onShow();
  virtual void onHide();
  virtual void onTimer();
  virtual void onCommand(int);
  virtual void onNotify(LPNMHDR, LPARAM);
  virtual void onContextMenu(HWND, int x, int y);
  virtual void onDestroy();

private:
  static WDL_DLGRET Proc(HWND, UINT, WPARAM, LPARAM);
  static DialogMap s_instances;

  const int m_template;
  bool m_isVisible;
  bool m_isEnabled;

  REAPER_PLUGIN_HINSTANCE m_instance;
  HWND m_parent;
  HWND m_handle;

  std::map<int, Control *> m_controls;
};

class LockDialog {
public:
  LockDialog(Dialog *dlg)
    : m_dialog(dlg), m_enabled(dlg->isEnabled())
  {
    if(m_enabled)
      m_dialog->disable();
  }

  ~LockDialog()
  {
    if(m_enabled)
      m_dialog->enable();
  }

private:
  Dialog *m_dialog;
  bool m_enabled;
};

#endif
