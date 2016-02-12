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
#include "errors.hpp"
#include "index.hpp"
#include "listview.hpp"
#include "menu.hpp"
#include "registry.hpp"
#include "remote.hpp"
#include "resource.hpp"
#include "richedit.hpp"
#include "tabbar.hpp"

#include <boost/algorithm/string/replace.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <sstream>

using namespace std;

enum { ACTION_HISTORY = 300 };

About::About(const Remote *remote, const RemoteIndex *index)
  : Dialog(IDD_ABOUT_DIALOG), m_remote(remote), m_index(index),
    m_currentCat(-255)
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

  m_packages->sortByColumn(0);
  m_packages->onActivate(bind(&About::packageHistory, this));

  m_installedFiles = getControl(IDC_LIST);

  m_tabs = createControl<TabBar>(IDC_TABS, TabBar::Tabs{
    {AUTO_STR("Description"), {m_about->handle()}},
    {AUTO_STR("Packages"), {m_cats->handle(), m_packages->handle()}},
    {AUTO_STR("Installed Files"), {m_installedFiles}},
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
  case IDC_WEBSITE:
    selectLink(id, m_websiteLinks);
    break;
  case IDC_DONATE:
    selectLink(id, m_donationLinks);
    break;
  case IDC_ENABLE:
    close(EnableResult);
    break;
  case ACTION_HISTORY:
    packageHistory();
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

void About::onContextMenu(HWND target, const int x, const int y)
{
  if(target != m_packages->handle())
    return;

  const int packageIndex = m_packages->currentIndex();

  if(packageIndex < 0)
    return;

  Menu menu;
  menu.addAction(AUTO_STR("Package &History"), ACTION_HISTORY);
  menu.show(x, y, handle());
}

void About::populate()
{
  auto_char title[255] = {};
  const auto_string &name = make_autostring(m_index->name());
  auto_snprintf(title, sizeof(title), AUTO_STR("About %s"), name.c_str());
  SetWindowText(handle(), title);

  m_websiteLinks = m_index->links(RemoteIndex::WebsiteLink);
  if(m_websiteLinks.empty())
    hide(getControl(IDC_WEBSITE));

  m_donationLinks = m_index->links(RemoteIndex::DonationLink);
  if(m_donationLinks.empty())
    hide(getControl(IDC_DONATE));

  if(m_remote->isEnabled())
    hide(getControl(IDC_ENABLE));

  if(!m_about->setRichText(m_index->aboutText())) {
    // if description is invalid or empty, don't display it
    m_tabs->removeTab(0);
    m_tabs->setCurrentIndex(0);
  }

  m_cats->addRow({AUTO_STR("<All Packages>")});

  for(const Category *cat : m_index->categories())
    m_cats->addRow({make_autostring(cat->name())});

  m_cats->sortByColumn(0);

  updatePackages();

  updateInstalledFiles();
}

void About::updatePackages()
{
  const int index = m_cats->currentIndex();

  // do nothing when the selection is cleared, except for the initial execution
  if(index == -1 && m_currentCat >= -1)
    return;

  // -1: all packages, >0 selected category
  const int catIndex = max(-1, index - 1);

  if(catIndex == m_currentCat)
    return;
  else if(catIndex < 0)
    m_packagesData = &m_index->packages();
  else
    m_packagesData = &m_index->category(catIndex)->packages();

  InhibitControl lock(m_packages);
  m_packages->clear();

  for(const Package *pkg : *m_packagesData) {
    const Version *lastVer = pkg->lastVersion();
    const auto_string &name = make_autostring(pkg->name());
    const auto_string &version = make_autostring(lastVer->name());
    const auto_string &author = make_autostring(lastVer->author());

    m_packages->addRow({name, version,
      author.empty() ? AUTO_STR("Unknown") : author});
  }

  m_currentCat = catIndex;
  m_packages->sort();
}

void About::updateInstalledFiles()
{
  set<Path> allFiles;

  try {
    Registry reg(Path::prefixCache("registry.db"));
    for(const Registry::Entry &entry : reg.getEntries(*m_remote)) {
      const set<Path> &files = reg.getFiles(entry);
      allFiles.insert(files.begin(), files.end());
    }
  }
  catch(const reapack_error &e) {
    const auto_string &desc = make_autostring(e.what());
    auto_char msg[255] = {};
    auto_snprintf(msg, sizeof(msg),
      AUTO_STR("The file list is currently unavailable.")
      AUTO_STR("Retry later when all installation task are completed.\r\n")
      AUTO_STR("\r\nError description: %s"),
      desc.c_str());
    SetWindowText(m_installedFiles, msg);
    return;
  }

  if(allFiles.empty()) {
    SetWindowText(m_installedFiles,
      AUTO_STR(
      "This repository does not own any file on your computer at this time.\r\n")

      AUTO_STR("It is either not yet installed or it does not provide ")
      AUTO_STR("any package compatible with your system."));
  }
  else {
    stringstream stream;

    for(const Path &path : allFiles)
      stream << path.join() << "\r\n";

    SetWindowText(m_installedFiles, make_autostring(stream.str()).c_str());
  }
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

void About::packageHistory()
{
  const int index = m_packages->currentIndex();

  if(index < 0)
    return;

  const Package *pkg = m_packagesData->at(index);
  Dialog::Show<History>(instance(), handle(), pkg);
}

History::History(const Package *pkg)
  : ReportDialog(), m_package(pkg)
{
}

void History::fillReport()
{
  SetWindowText(handle(), AUTO_STR("Package History"));
  SetWindowText(getControl(IDC_LABEL),
    make_autostring(m_package->name()).c_str());

  for(const Version *ver : m_package->versions() | boost::adaptors::reversed) {
    if(stream().tellp())
      stream() << NL;

    stream() << 'v' << ver->name();

    if(!ver->author().empty())
      stream() << " by " << ver->author();

    const string &date = ver->formattedDate();
    if(!date.empty())
      stream() << " – " << date;

    stream() << NL;

    const string &changelog = ver->changelog();
    if(changelog.empty())
      printChangelog("No changelog");
    else
      printChangelog(changelog);
  }
}
