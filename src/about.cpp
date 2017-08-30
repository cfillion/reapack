/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2017  Christian Fillion
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

#include "browser.hpp"
#include "config.hpp"
#include "errors.hpp"
#include "filesystem.hpp"
#include "index.hpp"
#include "listview.hpp"
#include "menu.hpp"
#include "ostream.hpp"
#include "reapack.hpp"
#include "registry.hpp"
#include "remote.hpp"
#include "report.hpp"
#include "resource.hpp"
#include "richedit.hpp"
#include "tabbar.hpp"
#include "transaction.hpp"

#include <boost/algorithm/string.hpp>
#include <iomanip>

using namespace std;

enum {
  ACTION_ABOUT_PKG = 300, ACTION_FIND_IN_BROWSER,
  ACTION_COPY_URL, ACTION_LOCATE
};

About::About()
  : Dialog(IDD_ABOUT_DIALOG)
{
}

void About::onInit()
{
  Dialog::onInit();

  m_tabs = createControl<TabBar>(IDC_TABS, this);
  m_desc = createControl<RichEdit>(IDC_ABOUT);

  m_menu = createControl<ListView>(IDC_MENU);
  m_menu->onSelect(bind(&About::updateList, this));

  m_list = createControl<ListView>(IDC_LIST);
  m_list->onContextMenu([=] (Menu &m, int i) { return m_delegate->fillContextMenu(m, i); });
  m_list->onActivate([=] { m_delegate->itemActivated(); });

  setMinimumSize({560, 300});
  setAnchor(m_tabs->handle(), AnchorRight | AnchorBottom);
  setAnchor(m_desc->handle(), AnchorRight | AnchorBottom);
  setAnchor(m_menu->handle(), AnchorBottom);
  setAnchor(m_list->handle(), AnchorRight | AnchorBottom);
  setAnchor(getControl(IDC_REPORT), AnchorRight | AnchorBottom);
  setAnchor(getControl(IDC_CHANGELOG), AnchorRight | AnchorBottom);
  setAnchor(getControl(IDC_WEBSITE), AnchorTop | AnchorBottom);
  setAnchor(getControl(IDC_DONATE), AnchorTop | AnchorBottom);
  setAnchor(getControl(IDC_SCREENSHOT), AnchorTop | AnchorBottom);
  setAnchor(getControl(IDC_ACTION), AnchorAll);
  setAnchor(getControl(IDOK), AnchorAll);

  auto data = m_serializer.read(g_reapack->config()->windowState.about, 1);
  restoreState(data);
}

void About::onClose()
{
  Serializer::Data data;
  saveState(data);
  g_reapack->config()->windowState.about = m_serializer.write(data);
}

void About::onCommand(const int id, int)
{
  switch(id) {
  case IDOK:
  case IDCANCEL:
    close();
    break;
  default:
    if(m_links.count(id))
      selectLink(id);
    else if(m_delegate)
      m_delegate->onCommand(id);
    break;
  }
}

bool About::onKeyDown(const int key, const int mods)
{
  if(GetFocus() != m_list->handle())
    return false;

  if(mods == CtrlModifier && key == 'C')
    m_delegate->itemCopy();
  else
    return false;

  return true;
}

void About::setDelegate(const DelegatePtr &delegate, const bool focus)
{
  if(m_delegate && delegate->data() == m_delegate->data()) {
    if(focus)
      setFocus(); // also calls show()
    return;
  }

  // prevent fast flickering on Windows
  InhibitControl block(handle());

  m_tabs->clear();
  m_menu->reset();
  m_menu->sortByColumn(0);
  m_list->reset();
  m_list->sortByColumn(0);

  m_delegate = nullptr;
  m_links.clear();

  const int controls[] = {
    IDC_ABOUT,
    IDC_MENU,
    IDC_LIST,
    IDC_CHANGELOG,
    IDC_REPORT,
    IDC_WEBSITE,
    IDC_SCREENSHOT,
    IDC_DONATE,
    IDC_ACTION,
  };

  for(const int control : controls)
    hide(getControl(control));

  m_delegate = delegate;
  m_delegate->init(this);
  m_menu->sort();

  m_currentIndex = -255;
  updateList();

  m_menu->autoSizeHeader();
  m_list->autoSizeHeader();

  if(focus) {
    show();
    m_tabs->setFocus();
  }
}

void About::setTitle(const String &what)
{
  Char title[255];
  snprintf(title, lengthof(title), L("About %s"), what.c_str());
  SetWindowText(handle(), title);
}

void About::setMetadata(const Metadata *metadata, const bool substitution)
{
  String aboutText(metadata->about());

  if(substitution) {
    boost::replace_all(aboutText, "[[REAPACK_VERSION]]", ReaPack::VERSION);
    boost::replace_all(aboutText, "[[REAPACK_BUILDTIME]]", ReaPack::BUILDTIME);
  }

  if(m_desc->setRichText(aboutText))
    m_tabs->addTab({L("About"), {m_desc->handle()}});

  const auto &getLinkControl = [](const Metadata::LinkType type) {
    switch(type) {
    case Metadata::WebsiteLink:
      return IDC_WEBSITE;
    case Metadata::DonationLink:
      return IDC_DONATE;
    case Metadata::ScreenshotLink:
      return IDC_SCREENSHOT;
    }

    return IDC_WEBSITE; // make MSVC happy
  };

  RECT rect;
  GetWindowRect(getControl(IDC_WEBSITE), &rect);
  ScreenToClient(handle(), (LPPOINT)&rect);
  ScreenToClient(handle(), ((LPPOINT)&rect)+1);

  const int shift = (rect.right - rect.left) + 4;

  for(const auto &pair : metadata->links()) {
    const int control = getLinkControl(pair.first);

    if(!m_links.count(control)) {
      HWND handle = getControl(control);
      setAnchorPos(handle, &rect.left, nullptr, &rect.right);
      show(handle);

      rect.left += shift;
      rect.right += shift;

      m_links[control] = {};
    }

    m_links[control].push_back(&pair.second);
  }

  onResize(); // update the position of link buttons
}

void About::setAction(const String &label)
{
  HWND btn = getControl(IDC_ACTION);
  SetWindowText(btn, label.c_str());
  show(btn);
}

void About::selectLink(const int ctrl)
{
  const auto &links = m_links[ctrl];
  const int count = (int)links.size();

  m_tabs->setFocus();

  if(count == 1) {
    openURL(links.front()->url);
    return;
  }

  Menu menu;

  for(int i = 0; i < count; i++) {
    const auto &name = boost::replace_all_copy(links[i]->name, "&", "&&");
    menu.addAction(name.c_str(), i | (ctrl << 8));
  }

  const int choice = menu.show(getControl(ctrl), handle());

  if(choice >> 8 == ctrl)
    openURL(links[choice & 0xff]->url);
}

void About::updateList()
{
  const int index = m_menu->currentIndex();

  // do nothing when the selection is cleared, except for the initial execution
  if((index < 0 && m_currentIndex != -255) || index == m_currentIndex)
    return;

  InhibitControl lock(m_list);
  m_list->clear();

  m_delegate->updateList(index);
  m_currentIndex = index;

  m_list->sort();
}

AboutIndexDelegate::AboutIndexDelegate(const IndexPtr &index)
  : m_index(index)
{
}

void AboutIndexDelegate::init(About *dialog)
{
  m_dialog = dialog;

  dialog->setTitle(m_index->name());
  dialog->setMetadata(m_index->metadata(), m_index->name() == L("ReaPack"));
  dialog->setAction(L("Install/update ") + m_index->name());

  dialog->tabs()->addTab({L("Packages"),
    {dialog->menu()->handle(), dialog->list()->handle()}});
  dialog->tabs()->addTab({L("Installed Files"),
    {dialog->getControl(IDC_REPORT)}});

  dialog->menu()->addColumn({L("Category"), 142});

  dialog->menu()->reserveRows(m_index->categories().size() + 1);
  dialog->menu()->createRow()->setCell(0, L("<All Packages>"));

  for(const Category *cat : m_index->categories())
    dialog->menu()->createRow()->setCell(0, cat->name());

  dialog->list()->addColumn({L("Package"), 382});
  dialog->list()->addColumn({L("Version"), 80, 0, ListView::VersionType});
  dialog->list()->addColumn({L("Author"), 90});

  initInstalledFiles();
}

void AboutIndexDelegate::initInstalledFiles()
{
  HWND report = m_dialog->getControl(IDC_REPORT);

  set<Registry::File> allFiles;

  try {
    Registry reg(Path::prefixRoot(Path::REGISTRY));
    for(const Registry::Entry &entry : reg.getEntries(m_index->name())) {
      const vector<Registry::File> &files = reg.getFiles(entry);
      allFiles.insert(files.begin(), files.end());
    }
  }
  catch(const reapack_error &e) {
    Char msg[255];
    snprintf(msg, lengthof(msg),
      L("The file list is currently unavailable.\x20")
      L("Retry later when all installation task are completed.\r\n")
      L("\r\nError description: %s"), e.what().c_str());
    SetWindowText(report, msg);
    return;
  }

  if(allFiles.empty()) {
    SetWindowText(report,
      L("This repository does not own any file on your computer at this time.\r\n")

      L("It is either not yet installed or it does not provide ")
      L("any package compatible with your system."));
  }
  else {
    StringStream stream;

    for(const Registry::File &file : allFiles) {
      stream << file.path.join();
      if(file.sections) // is this file registered in the action list?
        stream << L'*';
      stream << "\r\n";
    }

    SetWindowText(report, stream.str().c_str());
  }
}

void AboutIndexDelegate::updateList(const int index)
{
  // -1: all packages, >0 selected category
  const int catIndex = index - 1;

  const vector<const Package *> *packages;

  if(catIndex < 0)
    packages = &m_index->packages();
  else
    packages = &m_index->category(catIndex)->packages();

  m_dialog->list()->reserveRows(packages->size());

  for(const Package *pkg : *packages) {
    int c = 0;
    const Version *lastVer = pkg->lastVersion();

    auto row = m_dialog->list()->createRow((void *)pkg);
    row->setCell(c++, pkg->displayName());
    row->setCell(c++, lastVer->name().toString(), (void *)&lastVer->name());
    row->setCell(c++, lastVer->displayAuthor());
  }
}

bool AboutIndexDelegate::fillContextMenu(Menu &menu, const int index) const
{
  if(index < 0)
    return false;

  menu.addAction(L("Find in the &browser"), ACTION_FIND_IN_BROWSER);
  menu.addAction(L("About this &package"), ACTION_ABOUT_PKG);

  return true;
}

void AboutIndexDelegate::onCommand(const int id)
{
  switch(id) {
  case ACTION_FIND_IN_BROWSER:
    findInBrowser();
    break;
  case ACTION_ABOUT_PKG:
    aboutPackage();
    break;
  case IDC_ACTION:
    install();
    break;
  }
}

const Package *AboutIndexDelegate::currentPackage() const
{
  const int index = m_dialog->list()->currentIndex();

  if(index < 0)
    return nullptr;
  else
    return (const Package *)m_dialog->list()->row(index)->userData;
}

void AboutIndexDelegate::findInBrowser()
{
  Browser *browser = g_reapack->browsePackages();
  if(!browser)
    return;

  const Package *pkg = currentPackage();

  StringStreamO stream;
  stream << L'^' << quoted(pkg->displayName()) << L("$ ^") << quoted(m_index->name()) << L'$';
  browser->setFilter(stream.str());
}

void AboutIndexDelegate::aboutPackage()
{
  const Package *pkg = currentPackage();

  VersionName current;

  try {
    Registry reg(Path::prefixRoot(Path::REGISTRY));
    current = reg.getEntry(pkg).version;
  }
  catch(const reapack_error &) {}

  m_dialog->setDelegate(make_shared<AboutPackageDelegate>(pkg, current));
}

void AboutIndexDelegate::itemCopy()
{
  if(const Package *pkg = currentPackage())
    m_dialog->setClipboard(pkg->displayName());
}

void AboutIndexDelegate::install()
{
  enum { INSTALL_ALL = 80, UPDATE_ONLY };

  Menu menu;
  menu.addAction(L("Install all packages in this repository"), INSTALL_ALL);
  menu.addAction(L("Update installed packages only"), UPDATE_ONLY);

  const int choice = menu.show(m_dialog->getControl(IDC_ACTION), m_dialog->handle());

  if(!choice)
    return;

  Remote remote = g_reapack->remote(m_index->name());

  if(!remote) {
    // In case the user uninstalled the repository while this dialog was opened
    MessageBox(m_dialog->handle(),
      L("This repository cannot be found in your current configuration."),
      L("ReaPack"), MB_OK);
    return;
  }

  const InstallOpts &installOpts = g_reapack->config()->install;

  if(choice == INSTALL_ALL && boost::logic::indeterminate(remote.autoInstall())
      && !installOpts.autoInstall) {
    const int btn = MessageBox(m_dialog->handle(),
      L("Do you want ReaPack to install new packages from this repository")
      L(" when synchronizing in the future?\r\n\r\nThis setting can also be")
      L(" customized globally or on a per-repository basis in")
      L(" ReaPack > Manage repositories."),
      L("Install all packages in this repository"), MB_YESNOCANCEL);

    switch(btn) {
    case IDYES:
      remote.setAutoInstall(true);
      g_reapack->config()->remotes.add(remote);
      break;
    case IDCANCEL:
      return;
    }
  }

  Transaction *tx = g_reapack->setupTransaction();

  if(!tx)
    return;

  g_reapack->setRemoteEnabled(remote);

  tx->synchronize(remote, choice == INSTALL_ALL);
  tx->runTasks();
}

AboutPackageDelegate::AboutPackageDelegate(
    const Package *pkg, const VersionName &ver)
  : m_package(pkg), m_current(ver),
    m_index(pkg->category()->index()->shared_from_this())
{
}

void AboutPackageDelegate::init(About *dialog)
{
  m_dialog = dialog;

  dialog->setTitle(m_package->displayName());
  dialog->setMetadata(m_package->metadata());
  dialog->setAction(L("About ") + m_index->name());

  dialog->tabs()->addTab({L("History"),
    {dialog->menu()->handle(), dialog->getControl(IDC_CHANGELOG)}});
  dialog->tabs()->addTab({L("Contents"),
    {dialog->menu()->handle(), dialog->list()->handle()}});

  dialog->menu()->addColumn({L("Version"), 142, 0, ListView::VersionType});

  dialog->list()->addColumn({L("File"), 474});
  dialog->list()->addColumn({L("Action List"), 84});

  dialog->menu()->reserveRows(m_package->versions().size());

  for(const Version *ver : m_package->versions()) {
    auto row = dialog->menu()->createRow();
    row->setCell(0, ver->name().toString(), (void *)&ver->name());

    if(m_current == ver->name())
      dialog->menu()->select(row->index());
  }

  dialog->menu()->sortByColumn(0, ListView::DescendingOrder);

  if(!dialog->menu()->hasSelection())
    dialog->menu()->select(dialog->menu()->rowCount() - 1);
}

void AboutPackageDelegate::updateList(const int index)
{
  static const map<Source::Section, const Char *> sectionMap{
    {Source::MainSection, L("Main")},
    {Source::MIDIEditorSection, L("MIDI Editor")},
    {Source::MIDIInlineEditorSection, L("MIDI Inline Editor")},
  };

  if(index < 0)
    return;

  const Version *ver = m_package->version(index);
  OutputStream stream;
  stream << *ver;
  SetWindowText(m_dialog->getControl(IDC_CHANGELOG), stream.str().c_str());

  m_dialog->list()->reserveRows(ver->sources().size());

  for(const Source *src : ver->sources()) {
    int sections = src->sections();
    String actionList;

    if(sections) {
      vector<String> sectionNames;

      for(const auto &pair : sectionMap) {
        if(sections & pair.first) {
          sectionNames.push_back(pair.second);
          sections &= ~pair.first;
        }
      }

      if(sections) // In case we forgot to add a section to sectionMap!
        sectionNames.push_back(L("Other"));

      actionList = L("Yes (");
      actionList += boost::algorithm::join(sectionNames, L(", "));
      actionList += L')';
    }
    else
      actionList = L("No");

    int c = 0;
    auto row = m_dialog->list()->createRow((void *)src);
    row->setCell(c++, src->targetPath().join());
    row->setCell(c++, actionList);
  }
}

bool AboutPackageDelegate::fillContextMenu(Menu &menu, const int index) const
{
  if(index < 0)
    return false;

  auto src = (const Source *)m_dialog->list()->row(index)->userData;

  menu.addAction(L("Copy source URL"), ACTION_COPY_URL);
  menu.setEnabled(m_current.size() > 0 && FS::exists(src->targetPath()),
    menu.addAction(L("Locate in explorer/finder"), ACTION_LOCATE));

  return true;
}

void AboutPackageDelegate::onCommand(const int id)
{
  switch(id) {
  case IDC_ACTION:
    m_dialog->setDelegate(make_shared<AboutIndexDelegate>(m_index));
    break;
  case ACTION_COPY_URL:
    copySourceUrl();
    break;
  case ACTION_LOCATE:
    locate();
    break;
  }
}

const Source *AboutPackageDelegate::currentSource() const
{
  const int index = m_dialog->list()->currentIndex();

  if(index < 0)
    return nullptr;
  else
    return (const Source *)m_dialog->list()->row(index)->userData;
}

void AboutPackageDelegate::copySourceUrl()
{
  if(const Source *src = currentSource())
    m_dialog->setClipboard(src->url());
}

void AboutPackageDelegate::locate()
{
  if(const Source *src = currentSource()) {
    const Path &path = src->targetPath();

    if(!FS::exists(path))
      return;

    String arg(L("/select,\""));
    arg += Path::prefixRoot(path).join();
    arg += L'"';

    ShellExecute(nullptr, L("open"), L("explorer.exe"),
      arg.c_str(), nullptr, SW_SHOW);
  }
}
