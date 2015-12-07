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

  template<class T>
  static T *Create(REAPER_PLUGIN_HINSTANCE instance, HWND parent)
  {
    Dialog *dlg = new T();
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
  void setEnabled(const bool);

  void show();
  void hide();
  void center();

protected:
  Dialog(const int templateId);
  virtual ~Dialog();

  HWND getItem(const int idc);

  virtual void onInit();
  virtual void onShow();
  virtual void onHide();
  virtual void onTimer();
  virtual void onCommand(WPARAM, LPARAM);
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
