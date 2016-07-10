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
#include "ostream.hpp"
#include "reapack.hpp"
#include "registry.hpp"
#include "report.hpp"
#include "resource.hpp"
#include "richedit.hpp"
#include "tabbar.hpp"

#include <boost/algorithm/string/replace.hpp>

using namespace std;

enum { ACTION_ABOUT_PKG = 300 };

AboutDialog::AboutDialog(const Metadata *metadata)
  : Dialog(IDD_ABOUT_DIALOG), m_metadata(metadata), m_currentIndex(-255)
{
  RichEdit::Init();
}

void AboutDialog::onInit()
{
  Dialog::onInit();

  auto_char title[255] = {};
  const auto_string &name = make_autostring(what());
  auto_snprintf(title, auto_size(title), AUTO_STR("About %s"), name.c_str());
  SetWindowText(handle(), title);

  m_tabs = createControl<TabBar>(IDC_TABS, TabBar::Tabs{});

  string aboutText = m_metadata->about();
  if(name == AUTO_STR("ReaPack")) {
    boost::replace_all(aboutText, "[[REAPACK_VERSION]]", ReaPack::VERSION);
    boost::replace_all(aboutText, "[[REAPACK_BUILDTIME]]", ReaPack::BUILDTIME);
  }

  RichEdit *desc = createControl<RichEdit>(IDC_ABOUT);
  if(desc->setRichText(aboutText))
    tabs()->addTab({AUTO_STR("About"), {desc->handle()}});

  m_menu = createMenu();
  m_menu->sortByColumn(0);
  m_menu->onSelect(bind(&AboutDialog::callUpdateList, this));

  m_list = createList();
  m_list->sortByColumn(0);

  m_report = getControl(IDC_REPORT);

  populate();
  populateLinks();
  m_menu->sort();
  callUpdateList();

#ifdef LVSCW_AUTOSIZE_USEHEADER
  m_menu->resizeColumn(m_menu->columnCount() - 1, LVSCW_AUTOSIZE_USEHEADER);
  m_list->resizeColumn(m_list->columnCount() - 1, LVSCW_AUTOSIZE_USEHEADER);
#endif
}

void AboutDialog::populateLinks()
{
  const auto &getLinkControl = [](const Metadata::LinkType type) {
    switch(type) {
    case Metadata::WebsiteLink:
      return IDC_WEBSITE;
    case Metadata::DonationLink:
      return IDC_DONATE;
    }

    return IDC_WEBSITE; // make MSVC happy
  };

  RECT rect;
  GetWindowRect(getControl(IDC_WEBSITE), &rect);
  ScreenToClient(handle(), (LPPOINT)&rect);
  ScreenToClient(handle(), ((LPPOINT)&rect)+1);

  const int shift = (rect.right - rect.left) + 4;

  for(const auto &pair : m_metadata->links()) {
    const int control = getLinkControl(pair.first);
    if(!m_links.count(control)) {
      HWND handle = getControl(control);
      SetWindowPos(handle, nullptr, rect.left, rect.top, 0, 0,
        SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);

      show(handle);
      rect.left += shift;

      m_links[control] = {};
    }
    m_links[control].push_back(&pair.second);
  }
}

void AboutDialog::onCommand(const int id, int)
{
  switch(id) {
  case IDOK:
  case IDCANCEL:
    close();
    break;
  default:
    if(m_links.count(id))
      selectLink(id);
    else if(m_links.count(id >> 8))
      openLink(m_links[id >> 8][id & 0xff]);
    break;
  }
}

void AboutDialog::callUpdateList()
{
  const int index = m_menu->currentIndex();

  // do nothing when the selection is cleared, except for the initial execution
  if((index < 0 && m_currentIndex >= -1) || index == m_currentIndex)
    return;

  InhibitControl lock(m_list);
  m_list->clear();

  updateList(index);
  m_currentIndex = index;

  m_list->sort();
}

void AboutDialog::selectLink(const int ctrl)
{
  const auto &links = m_links[ctrl];
  const int count = (int)links.size();

  m_tabs->setFocus();

  if(count == 1) {
    openLink(links.front());
    return;
  }

  Menu menu;

  for(int i = 0; i < count; i++) {
    const string &name = boost::replace_all_copy(links[i]->name, "&", "&&");
    menu.addAction(make_autostring(name).c_str(), i | (ctrl << 8));
  }

  RECT rect;
  GetWindowRect(getControl(ctrl), &rect);
  menu.show(rect.left, rect.bottom - 1, handle());
}

void AboutDialog::openLink(const Link *link)
{
  const auto_string &url = make_autostring(link->url);
  ShellExecute(nullptr, AUTO_STR("open"), url.c_str(), nullptr, nullptr, SW_SHOW);
}

AboutRemote::AboutRemote(IndexPtr index)
  : AboutDialog(index->metadata()), m_index(index)
{
}

const string &AboutRemote::what() const
{
  return m_index->name();
}

ListView *AboutRemote::createMenu()
{
  return createControl<ListView>(IDC_MENU, ListView::Columns{
    {AUTO_STR("Category"), 142}
  });
}

ListView *AboutRemote::createList()
{
  return createControl<ListView>(IDC_LIST, ListView::Columns{
    {AUTO_STR("Name"), 382},
    {AUTO_STR("Version"), 80},
    {AUTO_STR("Author"), 90},
  });
}

void AboutRemote::onCommand(const int id, const int event)
{
  switch(id) {
  case IDC_INSTALL:
    close(InstallResult);
    break;
  case ACTION_ABOUT_PKG:
    aboutPackage();
    break;
  default:
    AboutDialog::onCommand(id, event);
    break;
  }
}

void AboutRemote::populate()
{
  HWND installBtn = getControl(IDC_INSTALL);
  auto_char btnLabel[32] = {};
  auto_snprintf(btnLabel, auto_size(btnLabel),
    AUTO_STR("Install/update %s"), make_autostring(what()).c_str());
  SetWindowText(installBtn, btnLabel);
  show(installBtn);

  tabs()->addTab({AUTO_STR("Packages"), {menu()->handle(), list()->handle()}});
  tabs()->addTab({AUTO_STR("Installed Files"), {report()}});

  list()->onActivate(bind(&AboutRemote::aboutPackage, this));

  menu()->addRow({AUTO_STR("<All Packages>")});

  for(const Category *cat : m_index->categories())
    menu()->addRow({make_autostring(cat->name())});

  updateInstalledFiles();

  list()->onContextMenu(bind(&AboutRemote::fillContextMenu,
    this, placeholders::_1, placeholders::_2));
}

void AboutRemote::updateList(const int index)
{
  // -1: all packages, >0 selected category
  const int catIndex = max(-1, index - 1);

  if(catIndex < 0)
    m_packagesData = &m_index->packages();
  else
    m_packagesData = &m_index->category(catIndex)->packages();

  for(const Package *pkg : *m_packagesData) {
    const Version *lastVer = pkg->lastVersion();
    const auto_string &name = make_autostring(pkg->name());
    const auto_string &version = make_autostring(lastVer->name());
    const auto_string &author = make_autostring(lastVer->displayAuthor());

    list()->addRow({name, version, author});
  }
}

void AboutRemote::updateInstalledFiles()
{
  set<Registry::File> allFiles;

  try {
    Registry reg(Path::prefixRoot(Path::REGISTRY));
    for(const Registry::Entry &entry : reg.getEntries(m_index->name())) {
      const vector<Registry::File> &files = reg.getFiles(entry);
      allFiles.insert(files.begin(), files.end());
    }
  }
  catch(const reapack_error &e) {
    const auto_string &desc = make_autostring(e.what());
    auto_char msg[255] = {};
    auto_snprintf(msg, auto_size(msg),
      AUTO_STR("The file list is currently unavailable.\x20")
      AUTO_STR("Retry later when all installation task are completed.\r\n")
      AUTO_STR("\r\nError description: %s"),
      desc.c_str());
    SetWindowText(report(), msg);
    return;
  }

  if(allFiles.empty()) {
    SetWindowText(report(),
      AUTO_STR(
      "This repository does not own any file on your computer at this time.\r\n")

      AUTO_STR("It is either not yet installed or it does not provide ")
      AUTO_STR("any package compatible with your system."));
  }
  else {
    stringstream stream;

    for(const Registry::File &file : allFiles) {
      stream << file.path.join();
      if(file.main)
        stream << '*';
      stream << "\r\n";
    }

    SetWindowText(report(), make_autostring(stream.str()).c_str());
  }
}

bool AboutRemote::fillContextMenu(Menu &menu, const int) const
{
  if(list()->currentIndex() < 0)
    return false;

  menu.addAction(AUTO_STR("About this &package"), ACTION_ABOUT_PKG);

  return true;
}

void AboutRemote::aboutPackage()
{
  const int index = list()->currentIndex();

  if(index < 0)
    return;

  const Package *pkg = m_packagesData->at(index);
  Dialog::Show<AboutPackage>(instance(), handle(), pkg);
}

AboutPackage::AboutPackage(const Package *pkg)
  : AboutDialog(new Metadata), m_package(pkg) // FIXME: remove `new Metadata` ASAP
{
}

const string &AboutPackage::what() const
{
  return m_package->name();
}

ListView *AboutPackage::createMenu()
{
  return createControl<ListView>(IDC_MENU, ListView::Columns{
    {AUTO_STR("Version"), 142}
  });
}

ListView *AboutPackage::createList()
{
  return createControl<ListView>(IDC_LIST, ListView::Columns{
    {AUTO_STR("File"), 251},
    {AUTO_STR("Source"), 251},
    {AUTO_STR("Main"), 50},
  });
}

void AboutPackage::populate()
{
  tabs()->addTab({AUTO_STR("History"), {menu()->handle(),
    report()}});
  tabs()->addTab({AUTO_STR("Contents"), {menu()->handle(),
    list()->handle()}});

  RECT rect;
  GetWindowRect(list()->handle(), &rect);
  ScreenToClient(handle(), (LPPOINT)&rect);
  ScreenToClient(handle(), ((LPPOINT)&rect)+1);

  SetWindowPos(report(), nullptr, rect.left, rect.top,
    rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER | SWP_NOACTIVATE);

  for(const Version *ver : m_package->versions())
    menu()->addRow({make_autostring(ver->name())});

  menu()->setSortCallback(0, [&] (const int a, const int b) {
    return m_package->version(a)->compare(*m_package->version(b));
  });
  menu()->sortByColumn(0, ListView::DescendingOrder);
  menu()->setSelected(menu()->rowCount() - 1, true);
}

void AboutPackage::updateList(const int index)
{
  if(index < 0)
    return;

  const Version *ver = m_package->version(index);
  OutputStream stream;
  stream << *ver;
  SetWindowText(report(), make_autostring(stream.str()).c_str());

  const auto &sources = ver->sources();
  for(auto it = sources.begin(); it != sources.end();) {
    const Path &path = it->first;
    const Source *src = it->second;

    list()->addRow({make_autostring(path.join()), make_autostring(src->url()),
      make_autostring(src->isMain() ? "Yes" : "No")});

    // skip duplicate files
    do { it++; } while(it != sources.end() && path == it->first);
  }
}
