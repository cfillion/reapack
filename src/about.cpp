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

#include "about.hpp"

#include "encoding.hpp"
#include "index.hpp"
#include "listview.hpp"
#include "resource.hpp"
#include "tabbar.hpp"

using namespace std;

About::About(RemoteIndex *index)
  : Dialog(IDD_ABOUT_DIALOG), m_index(index)
{
}

void About::onInit()
{
  m_about = getControl(IDC_ABOUT);

  m_cats = createControl<ListView>(IDC_CATEGORIES, ListView::Columns{
    {AUTO_STR("Category"), 140}
  });

  m_cats->onSelect(bind(&About::updatePackages, this, false));

  m_packages = createControl<ListView>(IDC_PACKAGES, ListView::Columns{
    {AUTO_STR("Name"), 350},
    {AUTO_STR("Version"), 90},
    {AUTO_STR("Author"), 100},
  });

  m_tabs = createControl<TabBar>(IDC_TABS, TabBar::Tabs{
    {AUTO_STR("Description"), {m_about}},
    {AUTO_STR("Packages"), {m_cats->handle(), m_packages->handle()}},
    {AUTO_STR("Installed Files"), {}},
  });

  populate();
}

void About::onCommand(const int id)
{
  switch(id) {
  case IDOK:
  case IDCANCEL:
    close();
    break;
  }
}

void About::populate()
{
  const auto_string &name = make_autostring(m_index->name());

  auto_char title[255] = {};
  auto_snprintf(title, sizeof(title), AUTO_STR("About %s"), name.c_str());

  SetWindowText(handle(), title);

  setAboutText(
    AUTO_STR("{\\rtf1\\ansi{\\fonttbl\\f0\\fswiss Helvetica;}\\f0\\pard\n")
    AUTO_STR("This is some {\\b bold} text.\\par\n")
    AUTO_STR("}")
  );

  m_cats->addRow({AUTO_STR("<All Categories>")});

  for(Category *cat : m_index->categories())
    m_cats->addRow({make_autostring(cat->name())});

  updatePackages(true);
}

void About::updatePackages(const bool force)
{
  const PackageList *pkgList;

  // -2: no selection, -1: all categories, >0 selected category
  const int catIndex = m_cats->currentIndex() - 1;

  if(catIndex == -2 && !force)
    return;
  else if(catIndex < 0)
    pkgList = &m_index->packages();
  else
    pkgList = &m_index->category(catIndex)->packages();

  m_packages->clear();

  for(Package *pkg : *pkgList) {
    const auto_string &name = make_autostring(pkg->name());
    const auto_string &lastVer = make_autostring(pkg->lastVersion()->name());
    m_packages->addRow({name, lastVer, AUTO_STR("John Doe")});
  }
}
