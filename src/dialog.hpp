/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015  Christian Fillion
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

  HWND handle() const { return m_handle; }

  bool isVisible() const { return m_isVisible; }
  void enable(HWND = 0);
  void disable(HWND = 0);
  void setEnabled(const bool, HWND = 0);

  void show();
  void hide();
  void close(const INT_PTR = 0);
  void center();
  void setFocus();

protected:
  Dialog(const int templateId);
  virtual ~Dialog();

  HWND getItem(const int idc);

  virtual void onInit() = 0;
  virtual void onShow();
  virtual void onHide();
  virtual void onTimer();
  virtual void onCommand(WPARAM, LPARAM);
  virtual void onNotify(LPNMHDR, LPARAM);
  virtual void onContextMenu(HWND, LPARAM);
  virtual void onDestroy();

private:
  static WDL_DLGRET Proc(HWND, UINT, WPARAM, LPARAM);
  static DialogMap s_instances;

  const int m_template;
  bool m_isVisible;

  HWND m_parent;
  HWND m_handle;
};
#endif
