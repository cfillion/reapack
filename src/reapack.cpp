/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2023  Christian Fillion
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

#include "reapack.hpp"

#include "about.hpp"
#include "api.hpp"
#include "buildinfo.hpp"
#include "config.hpp"
#include "download.hpp"
#include "errors.hpp"
#include "filesystem.hpp"
#include "index.hpp"
#include "manager.hpp"
#include "obsquery.hpp"
#include "progress.hpp"
#include "report.hpp"
#include "richedit.hpp"
#include "transaction.hpp"
#include "win32.hpp"

#include <cassert>

#include <reaper_plugin_functions.h>

ReaPack *ReaPack::s_instance = nullptr;

#ifdef _WIN32
// Removes temporary files that could not be removed by an installation task
// (eg. extensions dll that were in use by REAPER).
// Surely there must be a better way...
static void CleanupTempFiles()
{
  const Path &path = (Path::DATA + "*.tmp").prependRoot();
  const std::wstring &pattern = Win32::widen(path.join());

  WIN32_FIND_DATA fd = {};
  HANDLE handle = FindFirstFile(pattern.c_str(), &fd);

  if(handle == INVALID_HANDLE_VALUE)
    return;

  do {
    std::wstring file = pattern;
    file.replace(file.size() - 5, 5, fd.cFileName); // 5 == strlen("*.tmp")
    DeleteFile(file.c_str());
  } while(FindNextFile(handle, &fd));

  FindClose(handle);
}
#endif

Path ReaPack::resourcePath()
{
#ifdef _WIN32
  // convert from the current system codepage to UTF-8
  if(atof(GetAppVersion()) < 5.70)
    return Win32::ansi2utf8(GetResourcePath());
#endif

  return {GetResourcePath()};
}

ReaPack::ReaPack(REAPER_PLUGIN_HINSTANCE instance, HWND mainWindow)
  : m_instance(instance), m_mainWindow(mainWindow),
    m_useRootPath(resourcePath()), m_config(Path::CONFIG.prependRoot()), m_tx{}
{
  assert(!s_instance);
  s_instance = this;

  DownloadContext::GlobalInit();
  RichEdit::Init();

  createDirectories();
  registerSelf();
  setupActions();
  setupAPI();

  if(m_config.isFirstRun())
    manageRemotes();

#ifdef _WIN32
  CleanupTempFiles();
#endif
}

ReaPack::~ReaPack()
{
  DownloadContext::GlobalCleanup();

  s_instance = nullptr;
}

void ReaPack::setupActions()
{
  m_actions.add("REAPACK_SYNC", "ReaPack: Synchronize packages",
    std::bind(&ReaPack::synchronizeAll, this));

  m_actions.add("REAPACK_BROWSE", "ReaPack: Browse packages...",
    std::bind(&ReaPack::browsePackages, this));

  m_actions.add("REAPACK_UPLOAD", "ReaPack: Package editor",
    std::bind(&ReaPack::uploadPackage, this));

  m_actions.add("REAPACK_IMPORT", "ReaPack: Import repositories...",
    std::bind(&ReaPack::importRemote, this));

  m_actions.add("REAPACK_MANAGE", "ReaPack: Manage repositories...",
    std::bind(&ReaPack::manageRemotes, this));

  m_actions.add("REAPACK_ABOUT", "ReaPack: About...",
    std::bind(&ReaPack::aboutSelf, this));
}

void ReaPack::setupAPI()
{
  m_api.emplace_back(&API::AboutInstalledPackage);
  m_api.emplace_back(&API::AboutRepository);
  m_api.emplace_back(&API::AddSetRepository);
  m_api.emplace_back(&API::BrowsePackages);
  m_api.emplace_back(&API::CompareVersions);
  m_api.emplace_back(&API::EnumOwnedFiles);
  m_api.emplace_back(&API::FreeEntry);
  m_api.emplace_back(&API::GetEntryInfo);
  m_api.emplace_back(&API::GetOwner);
  m_api.emplace_back(&API::GetRepositoryInfo);
  m_api.emplace_back(&API::ProcessQueue);
}

void ReaPack::synchronizeAll()
{
  const std::vector<Remote> &remotes = m_config.remotes.getEnabled();

  if(remotes.empty()) {
    ShowMessageBox("No repository enabled, nothing to do!", "ReaPack", MB_OK);
    return;
  }

  Transaction *tx = setupTransaction();

  if(!tx)
    return;

  for(const Remote &remote : remotes)
    tx->synchronize(remote);

  tx->runTasks();
}

void ReaPack::addSetRemote(const Remote &remote)
{
  if(remote.isEnabled() && remote.autoInstall(m_config.install.autoInstall)) {
    const Remote &previous = m_config.remotes.get(remote.name());

    if(!previous || !previous.isEnabled() || previous.url() != remote.url()) {
      if(Transaction *tx = setupTransaction())
        tx->synchronize(remote);
    }
  }

  m_config.remotes.add(remote);
}

void ReaPack::uninstall(const Remote &remote)
{
  if(remote.isProtected())
    return;

  assert(m_tx);
  m_tx->uninstall(remote);

  m_tx->onFinish >> [=] {
    if(!m_tx->isCancelled())
      config()->remotes.remove(remote);
  };
}

void ReaPack::uploadPackage()
{
  Win32::shellExecute("https://reapack.com/upload");
}

void ReaPack::importRemote()
{
  const bool autoClose = m_manager == nullptr;

  manageRemotes();

  if(!m_manager->importRepo() && autoClose)
    m_manager->close();
}

void ReaPack::manageRemotes()
{
  if(m_manager) {
    m_manager->setFocus();
    return;
  }

  m_manager = Dialog::Create<Manager>(m_instance, m_mainWindow,
    [=](INT_PTR) { m_manager.reset(); });
  m_manager->show();
}

Remote ReaPack::remote(const std::string &name) const
{
  return m_config.remotes.get(name);
}

void ReaPack::about(const Remote &repo, const bool focus)
{
  Transaction *tx = setupTransaction();
  if(!tx)
    return;

  const std::vector<Remote> repos{repo};

  tx->fetchIndexes(repos);
  tx->onFinish >> [=] {
    const auto &indexes = tx->getIndexes(repos);
    if(!indexes.empty())
      about()->setDelegate(std::make_shared<AboutIndexDelegate>(indexes.front()), focus);
  };
  tx->runTasks();
}

void ReaPack::aboutSelf()
{
  about(remote("ReaPack"));
}

About *ReaPack::about(const bool instantiate)
{
  if(m_about)
    return m_about.get();
  else if(!instantiate)
    return nullptr;

  m_about = Dialog::Create<About>(m_instance, m_mainWindow,
    [=](INT_PTR) { m_about.reset(); });

  return m_about.get();
}

Browser *ReaPack::browsePackages()
{
  if(m_browser)
    m_browser->setFocus();
  else {
    m_browser = Dialog::Create<Browser>(m_instance, m_mainWindow,
      [=](INT_PTR) { m_browser.reset(); });
    m_browser->refresh();
  }

  return m_browser.get();
}

Transaction *ReaPack::setupTransaction()
{
  if(m_progress && m_progress->isVisible())
    m_progress->setFocus();

  if(m_tx)
    return m_tx;

  try {
    m_tx = new Transaction;
  }
  catch(const reapack_error &e) {
    Win32::messageBox(m_mainWindow, String::format(
      "The following error occurred while creating a transaction:\n\n%s",
      e.what()
    ).c_str(), "ReaPack", MB_OK);
    return nullptr;
  }

  assert(!m_progress);
  m_progress = Dialog::Create<Progress>(m_instance, m_mainWindow,
    nullptr, m_tx->threadPool());

  m_tx->onFinish >> [=] {
    m_progress.reset();

    if(!m_tx->isCancelled() && !m_tx->receipt()->empty()) {
      LockDialog managerLock(m_manager.get());
      LockDialog browserLock(m_browser.get());

      Dialog::Show<Report>(m_instance, m_mainWindow, m_tx->receipt());
    }
  };

  m_tx->setObsoleteHandler([=] (std::vector<Registry::Entry> &entries) {
    LockDialog aboutLock(m_about.get());
    LockDialog browserLock(m_browser.get());
    LockDialog managerLock(m_manager.get());
    LockDialog progressLock(m_progress.get());

    return Dialog::Show<ObsoleteQuery>(m_instance, m_mainWindow,
      &entries, &config()->install.promptObsolete) == IDOK;
  });

  m_tx->setCleanupHandler(std::bind(&ReaPack::teardownTransaction, this));

  return m_tx;
}

void ReaPack::teardownTransaction()
{
  const bool needRefresh = m_tx->receipt()->test(Receipt::RefreshBrowser);

  delete m_tx;
  m_tx = nullptr;

  // Update the browser only after the transaction is deleted because
  // it must be able to start a new one to load the indexes
  if(needRefresh)
    refreshBrowser();
}

void ReaPack::commitConfig(bool refresh)
{
  if(m_tx) {
    if(refresh) {
      m_tx->receipt()->setIndexChanged(); // force browser refresh
      m_tx->onFinish >> std::bind(&ReaPack::refreshManager, this);
    }
    m_tx->onFinish >> std::bind(&Config::write, &m_config);
    m_tx->runTasks();
  }
  else {
    if(refresh) {
      refreshManager();
      refreshBrowser();
    }
    m_config.write();
  }
}

void ReaPack::refreshManager()
{
  if(m_manager)
    m_manager->refresh();
}

void ReaPack::refreshBrowser()
{
  if(m_browser)
    m_browser->refresh();
}

void ReaPack::createDirectories()
{
  const Path &path = Path::CACHE;

  if(FS::mkdir(path))
    return;

  Win32::messageBox(Splash_GetWnd(), String::format(
    "ReaPack could not create %s! "
    "Please investigate or report this issue.\n\n"
    "Error description: %s",
    path.prependRoot().join().c_str(), FS::lastError()
  ).c_str(), "ReaPack", MB_OK);
}

void ReaPack::registerSelf()
{
  // hard-coding galore!
  Index ri("ReaPack");
  Category cat("Extensions", &ri);
  Package pkg(Package::ExtensionType, "ReaPack.ext", &cat);
  Version ver(REAPACK_VERSION, &pkg);
  ver.setAuthor("cfillion");
  ver.addSource(new Source(REAPACK_FILENAME, "dummy url", &ver));

  try {
    Registry reg(Path::REGISTRY.prependRoot());
    const Registry::Entry &entry = reg.getEntry(&pkg);
    if(entry && entry.version == ver.name())
      return; // avoid modifying the database file at every startup
    reg.push(&ver);
    reg.commit();
  }
  catch(const reapack_error &) {
    // Best to ignore the error for now. If something is wrong with the registry
    // we'll show a message once when the user really wants to interact with ReaPack.
    //
    // Right now the user is likely to just want to use REAPER without being bothered.
  }
}
