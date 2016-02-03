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
#include "menu.hpp"
#include "resource.hpp"
#include "richedit.hpp"
#include "tabbar.hpp"

#include <boost/algorithm/string/replace.hpp>

using namespace std;

About::About(const RemoteIndex *index)
  : Dialog(IDD_ABOUT_DIALOG), m_index(index), m_currentCat(-255)
{
  RichEdit::Init();
}

void About::onInit()
{
  m_about = createControl<RichEdit>(IDC_ABOUT);

  m_cats = createControl<ListView>(IDC_CATEGORIES, ListView::Columns{
    {AUTO_STR("Category"), 142}
  });

  m_cats->onSelect(bind(&About::updatePackages, this));

#ifdef _WIN32
  // dirty hacks...
  const int NAME_SIZE = 330;
#else
  const int NAME_SIZE = 382;
#endif

  m_packages = createControl<ListView>(IDC_PACKAGES, ListView::Columns{
    {AUTO_STR("Name"), NAME_SIZE},
    {AUTO_STR("Version"), 80},
    {AUTO_STR("Author"), 90},
  });

  m_tabs = createControl<TabBar>(IDC_TABS, TabBar::Tabs{
    {AUTO_STR("Description"), {m_about->handle()}},
    {AUTO_STR("Packages"), {m_cats->handle(), m_packages->handle()}},
    {AUTO_STR("Installed Files"), {}},
  });

  m_website = getControl(IDC_WEBSITE);
  m_donate = getControl(IDC_DONATE);

  populate();

#ifdef LVSCW_AUTOSIZE_USEHEADER
  m_cats->resizeColumn(0, LVSCW_AUTOSIZE_USEHEADER);
  m_packages->resizeColumn(2, LVSCW_AUTOSIZE_USEHEADER);
#endif
}

void About::onCommand(const int id)
{
  switch(id) {
  case IDC_WEBSITE:
    selectLink(id, m_websiteLinks);
    break;
  case IDC_DONATE:
    selectLink(id, m_donationLinks);
    break;
  case IDOK:
  case IDCANCEL:
    close();
    break;
  default:
    if(id >> 8 == IDC_WEBSITE)
      openLink(m_websiteLinks[id & 0xff]);
    else if(id >> 8 == IDC_DONATE)
      openLink(m_donationLinks[id & 0xff]);
  }
}

void About::populate()
{
  auto_char title[255] = {};
  const auto_string &name = make_autostring(m_index->name());
  auto_snprintf(title, sizeof(title), AUTO_STR("About %s"), name.c_str());
  SetWindowText(handle(), title);

  m_websiteLinks = m_index->links(RemoteIndex::WebsiteLink);
  if(m_websiteLinks.empty())
    hide(m_website);

  m_donationLinks = m_index->links(RemoteIndex::DonationLink);
  if(m_donationLinks.empty())
    hide(m_donate);

  if(!m_about->setRichText(m_index->aboutText())) {
    // if description is invalid or empty, don't display it
    m_tabs->removeTab(0);
    m_tabs->setCurrentIndex(0);
  }

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

  InhibitControl lock(m_packages);
  m_packages->clear();

  for(Package *pkg : *pkgList) {
    Version *lastVer = pkg->lastVersion();
    const auto_string &name = make_autostring(pkg->name());
    const auto_string &version = make_autostring(lastVer->name());
    const auto_string &author = make_autostring(lastVer->author());

    m_packages->addRow({name, version,
      author.empty() ? AUTO_STR("Unknown") : author});
  }

  m_currentCat = catIndex;
}

void About::selectLink(const int ctrl, const std::vector<const Link *> &links)
{
  const int count = (int)links.size();

  if(count > 1) {
    Menu menu;

    for(int i = 0; i < count; i++) {
      const string &name = boost::replace_all_copy(links[i]->name, "&", "&&");
      menu.addAction(make_autostring(name).c_str(), i | (ctrl << 8));
    }

    RECT rect;
    GetWindowRect(getControl(ctrl), &rect);
    menu.show(rect.left, rect.bottom - 1, handle());
  }
  else if(count == 1)
    openLink(links.front());

  m_tabs->setFocus();
}

void About::openLink(const Link *link)
{
  const auto_string &url = make_autostring(link->url);
  ShellExecute(nullptr, AUTO_STR("open"), url.c_str(), nullptr, nullptr, SW_SHOW);
}
