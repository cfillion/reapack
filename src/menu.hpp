#ifndef REAPACK_MENU_HPP
#define REAPACK_MENU_HPP

#include <swell/swell.h>

class Menu {
public:
  Menu(HMENU handle);

  void addAction(const char *label, const int commandId);
  void addSeparator();
  Menu addMenu(const char *label);

private:
  void append(MENUITEMINFO &);

  HMENU m_handle;
  unsigned int m_index;
};

#endif
