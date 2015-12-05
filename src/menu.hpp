#ifndef REAPACK_MENU_HPP
#define REAPACK_MENU_HPP

#ifdef _WIN32
#include <windows.h>
#else
#include <swell/swell.h>
#endif

class Menu {
public:
  Menu(HMENU handle);

  unsigned int size() { return m_size; }
  bool empty() const { return m_size == 0; }

  void addAction(const char *label, const int commandId);
  void addSeparator();
  Menu addMenu(const char *label);

private:
  void append(MENUITEMINFO &);

  HMENU m_handle;
  unsigned int m_size;
};

#endif
