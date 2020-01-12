/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2020  Christian Fillion
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

#include "serializer.hpp"

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <vector>

#include <reaper_plugin.h>
#include <WDL/wdltypes.h>
#include <WDL/wingui/wndsize.h>

class Control;

class Dialog {
public:
  typedef std::function<void (INT_PTR)> CloseHandler;

  enum Modality {
    Modeless,
    Modal,
  };

  enum AnchorFlags {
    AnchorLeft   = 1<<0,
    AnchorRight  = 1<<1,
    AnchorTop    = 1<<2,
    AnchorBottom = 1<<3,

    AnchorLeftRight = AnchorLeft | AnchorRight,
    AnchorTopBottom = AnchorTop | AnchorBottom,
    AnchorAll = AnchorLeftRight | AnchorTopBottom,
  };

  template<class T, class... Args>
  static std::unique_ptr<T> Create(REAPER_PLUGIN_HINSTANCE instance,
    HWND parent, const CloseHandler &closeHandler, Args&&... args)
  {
    auto dlg = std::make_unique<T>(args...);
    dlg->m_closeHandler = closeHandler;
    dlg->init(instance, parent, Dialog::Modeless);

    return dlg;
  }

  template<class T, class... Args>
  static INT_PTR Show(REAPER_PLUGIN_HINSTANCE instance, HWND parent, Args&&... args)
  {
    return std::make_unique<T>(args...)->init(instance, parent, Dialog::Modal);
  }

  INT_PTR init(REAPER_PLUGIN_HINSTANCE, HWND, const Modality);

  REAPER_PLUGIN_HINSTANCE instance() const { return m_instance; }
  HWND parent() const { return m_parent; }
  HWND handle() const { return m_handle; }

  void enable() { enable(m_handle); }
  void enable(HWND handle) { setEnabled(true, handle); }
  void disable() { disable(m_handle); }
  void disable(HWND handle) { setEnabled(false, handle); }
  void setEnabled(bool enable) { setEnabled(enable, m_handle); }
  void setEnabled(bool, HWND);
  bool isChecked(HWND) const;
  void setChecked(bool, HWND);

  bool isVisible() const;
  void show(HWND handle = nullptr) { setVisible(true, handle); }
  void hide(HWND handle = nullptr) { setVisible(false, handle); }
  void setVisible(bool, HWND = nullptr);

  void close(INT_PTR = 0);
  void center();
  void boundedMove(int x, int y);
  bool hasFocus() const;
  void setFocus();
  int startTimer(int elapse, int id = 0, bool replace = true);
  void stopTimer(int id);
  void setClipboard(const std::string &);
  void setClipboard(const std::vector<std::string> &);
  HWND getControl(int idc);
  void setAnchor(HWND, int flags);
  void setAnchorPos(HWND, const LONG *left = nullptr, const LONG *top = nullptr,
    const LONG *right = nullptr, const LONG *bottom = nullptr);
  const POINT &minimumSize() const { return m_minimumSize; }
  void setMinimumSize(const POINT &p) { m_minimumSize = p; }

  void restoreState(Serializer::Data &);
  void saveState(Serializer::Data &) const;

protected:
  enum Modifiers {
    AltModifier   = 1<<0,
    CtrlModifier  = 1<<1,
    ShiftModifier = 1<<2,
  };

  Dialog(int templateId);
  virtual ~Dialog();

  template<class T, class... Args>
  T *createControl(int id, Args&&... args)
  {
    if(m_controls.count(id))
      return nullptr;

    T *ctrl = new T(getControl(id), args...);
    m_controls.emplace(id, ctrl);

    return ctrl;
  }

  virtual void onInit();
  virtual void onClose();
  virtual void onTimer(int id);
  virtual void onCommand(int id, int event);
  virtual void onNotify(LPNMHDR, LPARAM);
  virtual void onContextMenu(HWND, int x, int y);
  virtual bool onKeyDown(int key, int mods);
  virtual void onResize();

private:
  static WDL_DLGRET Proc(HWND, UINT, WPARAM, LPARAM);
  static int HandleKey(MSG *, accelerator_register_t *);

  const int m_template;
  POINT m_minimumSize;
  WDL_WndSizer m_resizer;
  Modality m_mode;

  REAPER_PLUGIN_HINSTANCE m_instance;
  HWND m_parent;
  HWND m_handle;

  using ControlPtr = std::unique_ptr<Control>;
  std::map<int, ControlPtr> m_controls;
  std::set<int> m_timers;

  CloseHandler m_closeHandler;
  accelerator_register_t m_accel;
};

class LockDialog {
public:
  LockDialog(Dialog *dlg) : m_dialog(dlg)
  {
    if(m_dialog)
      m_dialog->disable();
  }

  ~LockDialog()
  {
    if(m_dialog)
      m_dialog->enable();
  }

private:
  Dialog *m_dialog;
};

#endif
