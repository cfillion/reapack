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

#include "reapack.hpp"

#include "about.hpp"
#include "api.hpp"
#include "browser.hpp"
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

#include <reaper_plugin_functions.h>

using namespace std;

const CharT *ReaPack::VERSION = L("1.2beta2");
const CharT *ReaPack::BUILDTIME = __DATE__ L(" ") __TIME__;

ReaPack *ReaPack::s_instance = nullptr;

#ifdef _WIN32
// Removes temporary files that could not be removed by an installation task
// (eg. extensions dll that were in use by REAPER).
// Surely there must be a better way...
static void CleanupTempFiles()
{
  const Path &path = Path::prefixRoot(Path::DATA + L("*.tmp"));
  const String &pattern = path.join();

  WIN32_FIND_DATA fd = {};
  HANDLE handle = FindFirstFile(pattern.c_str(), &fd);

  if(handle == INVALID_HANDLE_VALUE)
    return;

  do {
    String file = pattern;
    file.replace(file.size() - 5, 5, fd.cFileName); // 5 == strlen("*.tmp")
    DeleteFile(file.c_str());
  } while(FindNextFile(handle, &fd));

  FindClose(handle);
}
#endif

Path ReaPack::resourcePath()
{
#ifdef _WIN32
  // convert from the current system codepage to Unicode
  const char *path = GetResourcePath();
  return String(path, CP_ACP);
#else
  return {GetResourcePath()};
#endif
}

ReaPack::ReaPack(REAPER_PLUGIN_HINSTANCE instance)
  : syncAction(), browseAction(), importAction(), configAction(),
    m_tx(nullptr), m_progress(nullptr), m_browser(nullptr), m_manager(nullptr),
    m_about(nullptr), m_instance(instance), m_useRootPath(resourcePath())
{
  assert(!s_instance);
  s_instance = this;

  m_mainWindow = GetMainHwnd();

  DownloadContext::GlobalInit();
  RichEdit::Init();

  createDirectories();

  m_config = new Config;
  m_config->read(Path::prefixRoot(Path::CONFIG));

  if(m_config->isFirstRun())
    manageRemotes();

  registerSelf();

#ifdef _WIN32
  CleanupTempFiles();
#endif
}

ReaPack::~ReaPack()
{
  Dialog::DestroyAll();

  m_config->write();
  delete m_config;

  DownloadContext::GlobalCleanup();

  s_instance = nullptr;
}

int ReaPack::setupAction(const char *name, const ActionCallback &callback)
{
  const int id = plugin_register("command_id", (void *)name);
  m_actions[id] = callback;

  return id;
}

int ReaPack::setupAction(const char *name, const char *desc,
  gaccel_register_t *action, const ActionCallback &callback)
{
  const int id = setupAction(name, callback);

  action->desc = desc;
  action->accel.cmd = id;

  plugin_register("gaccel", action);

  return id;
}

bool ReaPack::execActions(const int id, const int)
{
  if(!m_actions.count(id))
    return false;

  m_actions.at(id)();

  return true;
}

void ReaPack::setupAPI(const APIFunc *func)
{
  m_api.push_back(std::make_unique<APIDef>(func));
}

void ReaPack::synchronizeAll()
{
  const vector<Remote> &remotes = m_config->remotes.getEnabled();

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

bool ReaPack::addSetRemote(const Remote &remote, const bool enable)
{
  if(remote.isEnabled() != enable) {
    if(Transaction *tx = setupTransaction()) {
      setRemoteEnabled(remote, enable); // adds the new remote to the list

      if(enable)
        tx->synchronize(remote);

      return true;
    }
    else
      return false;
  }

  m_config->remotes.add(remote);

  if(m_config->install.autoInstall) {
    if(Transaction *tx = setupTransaction()) {
      tx->synchronize(remote);
      return true;
    }
  }

  return true;
}

void ReaPack::setRemoteEnabled(const Remote &remote, const bool enable)
{
  assert(m_tx);

  Remote copy(remote);
  copy.setEnabled(enable);

  m_tx->registerAll(copy);

  m_tx->onFinish([=] {
    if(m_tx->isCancelled())
      return;

    m_config->remotes.add(copy);
  });
}

void ReaPack::uninstall(const Remote &remote)
{
  if(remote.isProtected())
    return;

  assert(m_tx);
  m_tx->uninstall(remote);

  m_tx->onFinish([=] {
    if(!m_tx->isCancelled())
      m_config->remotes.remove(remote);
  });
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

  m_manager = Dialog::Create<Manager>(m_instance, m_mainWindow);
  m_manager->show();

  m_manager->setCloseHandler([=] (INT_PTR) {
    Dialog::Destroy(m_manager);
    m_manager = nullptr;
  });
}

Remote ReaPack::remote(const String &name) const
{
  return m_config->remotes.get(name);
}

void ReaPack::about(const Remote &repo, const bool focus)
{
  Transaction *tx = setupTransaction();
  if(!tx)
    return;

  const vector<Remote> repos = {repo};

  tx->fetchIndexes(repos);
  tx->onFinish([=] {
    const auto &indexes = tx->getIndexes(repos);
    if(!indexes.empty())
      about()->setDelegate(make_shared<AboutIndexDelegate>(indexes.front()), focus);
  });
  tx->runTasks();
}

void ReaPack::aboutSelf()
{
  about(remote(L("ReaPack")));
}

About *ReaPack::about(const bool instantiate)
{
  if(m_about)
    return m_about;
  else if(!instantiate)
    return nullptr;

  m_about = Dialog::Create<About>(m_instance, m_mainWindow);

  m_about->setCloseHandler([=] (INT_PTR) {
    Dialog::Destroy(m_about);
    m_about = nullptr;
  });

  return m_about;
}

Browser *ReaPack::browsePackages()
{
  if(m_browser) {
    m_browser->setFocus();
    return m_browser;
  }
  else if(m_tx) {
    ShowMessageBox(
      "This feature cannot be used while packages are being installed. "
      "Try again later.", "Browse packages", MB_OK
    );
    return nullptr;
  }

  m_browser = Dialog::Create<Browser>(m_instance, m_mainWindow);
  m_browser->refresh();
  m_browser->setCloseHandler([=] (INT_PTR) {
    Dialog::Destroy(m_browser);
    m_browser = nullptr;
  });

  return m_browser;
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
    CharT msg[512];
    snprintf(msg, lengthof(msg),
      L("The following error occurred while creating a transaction:\n\n%s"),
      e.what().c_str()
    );

    MessageBox(m_mainWindow, msg, L("ReaPack"), MB_OK);
    return nullptr;
  }

  assert(!m_progress);
  m_progress = Dialog::Create<Progress>(m_instance, m_mainWindow, m_tx->threadPool());

  m_tx->onFinish([=] {
    Dialog::Destroy(m_progress);
    m_progress = nullptr;

    if(!m_tx->isCancelled() && !m_tx->receipt()->empty()) {
      LockDialog managerLock(m_manager);
      LockDialog browserLock(m_browser);

      Dialog::Show<Report>(m_instance, m_mainWindow, *m_tx->receipt());
    }
  });

  m_tx->setObsoleteHandler([=] (vector<Registry::Entry> &entries) {
    LockDialog progressLock(m_progress);
    LockDialog managerLock(m_manager);
    LockDialog browserLock(m_browser);

    return Dialog::Show<ObsoleteQuery>(m_instance, m_mainWindow,
      &entries, &m_config->install.promptObsolete) == IDOK;
  });

  m_tx->setCleanupHandler(bind(&ReaPack::teardownTransaction, this));

  return m_tx;
}

void ReaPack::teardownTransaction()
{
  delete m_tx;
  m_tx = nullptr;

  // Update the browser only after the transaction is deleted because
  // it must be able to start a new one to load the indexes
  refreshBrowser();
}

void ReaPack::commitConfig()
{
  if(m_tx) {
    m_tx->onFinish(bind(&ReaPack::refreshManager, this));
    m_tx->onFinish(bind(&Config::write, m_config));
    m_tx->runTasks();
  }
  else {
    refreshManager();
    refreshBrowser();
    m_config->write();
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

  CharT msg[255];
  snprintf(msg, lengthof(msg),
    L("ReaPack could not create %s! ")
    L("Please investigate or report this issue.\n\n")
    L("Error description: %s"),
    Path::prefixRoot(path).join().c_str(), FS::lastError().c_str());

  MessageBox(Splash_GetWnd(), msg, L("ReaPack"), MB_OK);
}

void ReaPack::registerSelf()
{
  // hard-coding galore!
  Index ri(L("ReaPack"));
  Category cat(L("Extensions"), &ri);
  Package pkg(Package::ExtensionType, L("ReaPack.ext"), &cat);
  Version ver(VERSION, &pkg);
  ver.setAuthor(L("cfillion"));
  ver.addSource(new Source(REAPACK_FILE, L("dummy url"), &ver));

  try {
    Registry reg(Path::prefixRoot(Path::REGISTRY));
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
