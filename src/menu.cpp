#include "menu.hpp"

Menu::Menu(HMENU handle)
  : m_handle(handle)
{
  m_size = GetMenuItemCount(m_handle);

  if(!empty())
    addSeparator();
}

void Menu::addAction(const char *label, const int commandId)
{
  MENUITEMINFO mii{};
  mii.cbSize = sizeof(MENUITEMINFO);

  mii.fMask |= MIIM_TYPE;
  mii.fType = MFT_STRING;
  mii.dwTypeData = const_cast<char *>(label);

  mii.fMask |= MIIM_ID;
  mii.wID = commandId;

  append(mii);
}

void Menu::addSeparator()
{
  MENUITEMINFO mii{};
  mii.cbSize = sizeof(MENUITEMINFO);

  mii.fMask = MIIM_TYPE;
  mii.fType = MFT_SEPARATOR;

  append(mii);
}

Menu Menu::addMenu(const char *label)
{
  MENUITEMINFO mii{};
  mii.cbSize = sizeof(MENUITEMINFO);

  mii.fMask |= MIIM_TYPE;
  mii.fType = MFT_STRING;
  mii.dwTypeData = const_cast<char *>(label);

  mii.fMask |= MIIM_SUBMENU;
  mii.hSubMenu = CreatePopupMenu();

  append(mii);

  return Menu(mii.hSubMenu);
}

void Menu::append(MENUITEMINFO &mii)
{
  InsertMenuItem(m_handle, m_size++, true, &mii);
}
