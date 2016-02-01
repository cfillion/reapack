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
#include "richedit.hpp"
#include "tabbar.hpp"

using namespace std;

About::About(RemoteIndex *index)
  : Dialog(IDD_ABOUT_DIALOG), m_index(index), m_currentCat(-255)
{
  RichEdit::Init();
}

void About::onInit()
{
  m_about = createControl<RichEdit>(IDC_ABOUT);

  m_cats = createControl<ListView>(IDC_CATEGORIES, ListView::Columns{
    {AUTO_STR("Category"), 200}
  });

  m_cats->onSelect(bind(&About::updatePackages, this));

#ifdef _WIN32
  const int NAME_SIZE = 330;
#else
  const int NAME_SIZE = 380;
#endif

  m_packages = createControl<ListView>(IDC_PACKAGES, ListView::Columns{
    {AUTO_STR("Name"), NAME_SIZE},
    {AUTO_STR("Version"), 80},
    {AUTO_STR("Author"), 100},
  });

  m_tabs = createControl<TabBar>(IDC_TABS, TabBar::Tabs{
    {AUTO_STR("Description"), {m_about->handle()}},
    {AUTO_STR("Packages"), {m_cats->handle(), m_packages->handle()}},
    {AUTO_STR("Installed Files"), {}},
  });

  populate();

#ifdef LVSCW_AUTOSIZE_USEHEADER
  m_cats->resizeColumn(0, LVSCW_AUTOSIZE_USEHEADER);
  m_packages->resizeColumn(2, LVSCW_AUTOSIZE_USEHEADER);
#endif
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

  m_about->setRichText(
    "{\\rtf1\\ansi\\ansicpg1252\\cocoartf1348\\cocoasubrtf170\n"
    "{\\fonttbl\\f0\\fnil\\fcharset134 STHeitiSC-Light;}\n"
    "{\\colortbl;\\red255\\green255\\blue255;}\n"
    "\\margl1440\\margr1440\\vieww10800\\viewh8400\\viewkind0\n"
    "\\pard\\tx566\\tx1133\\tx1700\\tx2267\\tx2834\\tx3401\\tx3968\\tx4535\\tx5102\\tx5669\\tx6236\\tx6803\\pardirnatural\n"
    "\\f0\\fs24 \\cf0 http://perdu.com test"
    "{\\field{\\*\\fldinst{HYPERLINK \"https://msdn.microsoft.com/en-us/library/windows/desktop/bb787974%28v=vs.85%29.aspx\"}}{\\fldrslt \\f0\\fs24 \\cf0 \\'d0\\'c2\\'ca\\'c0\\'bd\\'e7\\'a4\\'e8\\'a4\\'ea}}}\n"
  );

  m_cats->addRow({AUTO_STR("<All Categories>")});

  for(Category *cat : m_index->categories())
    m_cats->addRow({make_autostring(cat->name())});

  updatePackages();
}

void About::updatePackages()
{
  const int index = m_cats->currentIndex();

  // do nothing when the selection is cleared, except for the initial execution
  if(index == -1 && m_currentCat >= -1)
    return;

  // -1: all categories, >0 selected category
  const int catIndex = max(-1, index - 1);
  const PackageList *pkgList;

  if(catIndex == m_currentCat)
    return;
  else if(catIndex < 0)
    pkgList = &m_index->packages();
  else
    pkgList = &m_index->category(catIndex)->packages();

  m_packages->inhibitRedraw(true);
  m_packages->clear();

  for(Package *pkg : *pkgList) {
    const auto_string &name = make_autostring(pkg->name());
    const auto_string &lastVer = make_autostring(pkg->lastVersion()->name());
    m_packages->addRow({name, lastVer, AUTO_STR("John Doe")});
  }

  m_currentCat = catIndex;
  m_packages->inhibitRedraw(false);
}
