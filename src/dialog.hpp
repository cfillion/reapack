#ifndef REAPACK_DIALOG_HPP
#define REAPACK_DIALOG_HPP

#include <map>

#include <wdltypes.h>
#include <reaper_plugin.h>

class Dialog;
typedef std::map<HWND, Dialog *> DialogMap;

class Dialog {
public:
  template<typename T>
  static T *Create(REAPER_PLUGIN_HINSTANCE instance, HWND parent)
  {
    Dialog *dlg = new T();
    dlg->init(instance, parent);

    return dynamic_cast<T *>(dlg);
  }

  static void Destroy(Dialog *);

  void init(REAPER_PLUGIN_HINSTANCE, HWND);

  HWND handle() const { return m_handle; }

  bool isVisible() const { return m_isVisible; }
  void setEnabled(const bool);

  void show();
  void hide();
  void center();

protected:
  Dialog(const int templateId);
  virtual ~Dialog();

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
  HWND m_parent;
  HWND m_handle;

  bool m_isVisible;
};
#endif
