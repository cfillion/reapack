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

#include "reapack.hpp"

#include "about.hpp"
#include "cleanup.hpp"
#include "config.hpp"
#include "errors.hpp"
#include "filesystem.hpp"
#include "import.hpp"
#include "index.hpp"
#include "manager.hpp"
#include "progress.hpp"
#include "report.hpp"
#include "transaction.hpp"

#include <reaper_plugin_functions.h>

using namespace std;

const string ReaPack::VERSION = "0.8-beta";
const string ReaPack::BUILDTIME = __DATE__ " " __TIME__;

#ifdef _WIN32
// Removes temporary files that could not be removed by an installation task
// (eg. extensions dll that were in use by REAPER).
// Surely there must be a better way...
static void CleanupTempFiles()
{
  const Path &path = Path::prefixRoot(Path::CACHE + "*.tmp");
  const auto_string &pattern = make_autostring(path.join());

  WIN32_FIND_DATA fd = {};
  HANDLE handle = FindFirstFile(pattern.c_str(), &fd);

  if(handle == INVALID_HANDLE_VALUE)
    return;

  do {
    auto_string file = pattern;
    file.replace(file.size() - 5, 5, fd.cFileName); // 5 == strlen("*.tmp")
    DeleteFile(file.c_str());
  } while(FindNextFile(handle, &fd));

  FindClose(handle);
}
#endif

ReaPack::ReaPack(REAPER_PLUGIN_HINSTANCE instance)
  : syncAction(), cleanupAction(), importAction(), configAction(),
    m_transaction(nullptr), m_progress(nullptr), m_manager(nullptr),
    m_import(nullptr), m_cleanup(nullptr), m_instance(instance)
{
  m_mainWindow = GetMainHwnd();
  m_useRootPath = new UseRootPath(GetResourcePath());

  Download::Init();

  FS::mkdir(Path::CACHE);

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
  m_config->write();
  delete m_config;

  Dialog::DestroyAll();

  Download::Cleanup();

  delete m_useRootPath;
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

void ReaPack::synchronizeAll()
{
  const vector<Remote> &remotes = m_config->remotes()->getEnabled();

  if(remotes.empty()) {
    ShowMessageBox("No remote repository enabled, nothing to do!",
      "ReaPack", 0);

    return;
  }

  // do nothing is a transation is already running
  Transaction *t = createTransaction();

  if(!t)
    return;

  for(const Remote &remote : remotes)
    t->synchronize(remote);

  t->runTasks();
}

void ReaPack::setRemoteEnabled(const Remote &original, const bool enable)
{
  Remote remote(original);
  remote.setEnabled(enable);

  const auto apply = [=] {
    m_config->remotes()->add(remote);

    if(m_manager)
      m_manager->refresh();
  };

  if(!hitchhikeTransaction()) {
    apply();
    return;
  }

  m_transaction->registerAll(remote);

  m_transaction->onFinish([=] {
    if(!m_transaction->isCancelled())
      apply();
  });
}

void ReaPack::uninstall(const Remote &remote)
{
  if(remote.isProtected())
    return;

  const auto apply = [=] {
    m_config->remotes()->remove(remote);
  };

  if(!hitchhikeTransaction()) {
    apply();
    return;
  }

  m_transaction->uninstall(remote);

  m_transaction->onFinish([=] {
    if(!m_transaction->isCancelled())
      apply();
  });
}

void ReaPack::uninstall(const Registry::Entry &entry)
{
  if(hitchhikeTransaction())
    m_transaction->uninstall(entry);
}

void ReaPack::importRemote()
{
  if(m_import) {
    m_import->setFocus();
    return;
  }

  m_import = Dialog::Create<Import>(m_instance, m_mainWindow, this);
  m_import->show();
  m_import->setCloseHandler([=] (INT_PTR) {
    Dialog::Destroy(m_import);
    m_import = nullptr;
  });
}

void ReaPack::import(const Remote &remote)
{
  RemoteList *remotes = m_config->remotes();

  const Remote &existing = remotes->get(remote.name());

  if(!existing.isNull()) {
    if(existing.isProtected()) {
      ShowMessageBox(
        "This remote is protected and cannot be overwritten.", Import::TITLE, MB_OK);

      return;
    }
    else if(existing.url() != remote.url()) {
      const int button = ShowMessageBox(
        "This remote is already configured.\r\n"
        "Do you want to overwrite the current remote?"
        , Import::TITLE, MB_YESNO);

      if(button != IDYES)
        return;
    }
    else if(existing.isEnabled()) {
      ShowMessageBox(
        "This remote is already configured.\r\n"
        "Nothing to do!"
      , Import::TITLE, MB_OK);

      return;
    }
    else {
      enable(existing);
      m_config->write();

      return;
    }
  }

  remotes->add(remote);
  m_config->write();

  if(m_manager)
    m_manager->refresh();

  const string msg = remote.name() +
    " has been successfully imported into your repository list.";
  ShowMessageBox(msg.c_str(), Import::TITLE, MB_OK);
}

void ReaPack::manageRemotes()
{
  if(m_manager) {
    m_manager->setFocus();
    return;
  }

  m_manager = Dialog::Create<Manager>(m_instance, m_mainWindow, this);
  m_manager->refresh();
  m_manager->show();

  m_manager->setCloseHandler([=] (INT_PTR) {
    Dialog::Destroy(m_manager);
    m_manager = nullptr;
  });
}

void ReaPack::aboutSelf()
{
  about(m_config->remotes()->get("ReaPack"), m_mainWindow);
}

void ReaPack::about(const Remote &remote, HWND parent)
{
  if(remote.isNull())
    return;

  fetchIndex(remote, [=] (IndexPtr index) {
    const auto ret = Dialog::Show<About>(m_instance, parent, index);

    if(ret == About::InstallResult) {
      enable(remote);
      if(m_transaction)
        m_transaction->synchronize(remote);
    }
  }, parent);
}

void ReaPack::cleanupPackages()
{
  if(m_cleanup) {
    m_cleanup->setFocus();
    return;
  }
  else if(m_transaction) {
    ShowMessageBox(
      "This feature cannot be used while packages are being installed. "
      "Try again later.", "Clean up packages", MB_OK
    );
    return;
  }

  const vector<Remote> &remotes = m_config->remotes()->getEnabled();

  fetchIndexes(remotes, [=] (const vector<IndexPtr> &indexes) {
    m_cleanup = Dialog::Create<Cleanup>(m_instance, m_mainWindow, indexes, this);
    m_cleanup->show();
    m_cleanup->setCloseHandler([=] (INT_PTR) {
      Dialog::Destroy(m_cleanup);
      m_cleanup = nullptr;
    });
  });
}

void ReaPack::fetchIndex(const Remote &remote,
  const IndexCallback &callback, HWND parent)
{
  fetchIndexes({remote}, [=] (const vector<IndexPtr> &indexes) {
    callback(indexes.front());
  }, parent);
}

void ReaPack::fetchIndexes(const vector<Remote> &remotes,
  const IndexesCallback &callback, HWND parent)
{
  if(!parent)
    parent = m_mainWindow;

  DownloadQueue *queue = new DownloadQueue;
  Dialog *progress = Dialog::Create<Progress>(m_instance, m_mainWindow, queue);

  auto load = [=] {
    Dialog::Destroy(progress);
    delete queue;

    vector<IndexPtr> indexes;

    for(const Remote &remote : remotes) {
      if(!FS::exists(Index::pathFor(remote.name())))
        continue;

      IndexPtr index = loadIndex(remote, parent);

      if(index)
        indexes.push_back(index);
    }

    callback(indexes);
  };

  queue->onDone(load);

  // I don't know why, but at least on OSX giving the manager window handle
  // (in `parent`) to the progress dialog prevents it from being shown at all
  // while still taking the focus away from the manager dialog.

  for(const Remote &remote : remotes)
    fetchIndex(remote, queue, parent);

  if(queue->idle()) {
    load();
  }
}

void ReaPack::fetchIndex(const Remote &remote, DownloadQueue *queue, HWND parent)
{
  Download *dl = Index::fetch(remote);

  if(!dl)
    return;

  const auto warn = [=] (const string &desc, const auto_char *title) {
    auto_char msg[512] = {};
    auto_snprintf(msg, sizeof(msg),
      AUTO_STR("ReaPack could not download %s's index.\n\n")

      AUTO_STR("Try again later. ")
      AUTO_STR("If the problem persist, contact the repository maintainer.\n\n")

      AUTO_STR("[Error description: %s]"),
      make_autostring(remote.name()).c_str(), make_autostring(desc).c_str()
    );

    MessageBox(parent, msg, title, MB_OK);
  };

  dl->onFinish([=] {
    const Path &path = Index::pathFor(remote.name());

    switch(dl->state()) {
    case Download::Success:
      if(!FS::write(path, dl->contents()))
        warn(FS::lastError(), AUTO_STR("Write Failed"));
      break;
    case Download::Failure:
      if(!FS::exists(path))
        warn(dl->contents(), AUTO_STR("Download Failed"));
      break;
    default:
      break;
    }
  });

  queue->push(dl);
}

IndexPtr ReaPack::loadIndex(const Remote &remote, HWND parent)
{
  try {
    return Index::load(remote.name());
  }
  catch(const reapack_error &e) {
    const auto_string &desc = make_autostring(e.what());

    auto_char msg[512] = {};
    auto_snprintf(msg, sizeof(msg),
      AUTO_STR("ReaPack could not read %s's index.\n\n")

      AUTO_STR("Synchronize your packages and try again later.\n")
      AUTO_STR("If the problem persist, contact the repository maintainer.\n\n")

      AUTO_STR("[Error description: %s]"),
      make_autostring(remote.name()).c_str(), desc.c_str()
    );

    MessageBox(parent, msg, AUTO_STR("ReaPack"), MB_OK);
  }

  return nullptr;
}

Transaction *ReaPack::createTransaction()
{
  if(m_progress && m_progress->isVisible())
    m_progress->setFocus();

  if(m_transaction)
    return nullptr;

  try {
    m_transaction = new Transaction;
  }
  catch(const reapack_error &e) {
    ShowMessageBox(e.what(), "ReaPack – Fatal Error", 0);
    return nullptr;
  }

  assert(!m_progress);
  m_progress = Dialog::Create<Progress>(m_instance, m_mainWindow,
    m_transaction->downloadQueue());

  m_transaction->onFinish([=] {
    Dialog::Destroy(m_progress);
    m_progress = nullptr;

    const Receipt *receipt = m_transaction->receipt();

    if(m_transaction->isCancelled() || !receipt->isEnabled())
      return;

    LockDialog managerLock(m_manager);
    LockDialog cleanupLock(m_cleanup);

    if(m_transaction->taskCount() == 0 && !receipt->hasErrors())
      ShowMessageBox("Nothing to do!", "ReaPack", 0);
    else
      Dialog::Show<Report>(m_instance, m_mainWindow, receipt);
  });

  m_transaction->setCleanupHandler([=] {
    delete m_transaction;
    m_transaction = nullptr;
  });

  return m_transaction;
}

bool ReaPack::hitchhikeTransaction()
{
  if(m_transaction)
    return true;
  else
    return createTransaction() != nullptr;
}

void ReaPack::runTasks()
{
  if(m_transaction)
    m_transaction->runTasks();
}

void ReaPack::registerSelf()
{
  // hard-coding galore!
  Index ri("ReaPack");
  Category cat("Extensions", &ri);
  Package pkg(Package::ExtensionType, "ReaPack.ext", &cat);
  Version ver(VERSION, &pkg);
  ver.addSource(new Source(Source::GenericPlatform,
    REAPACK_FILE, "dummy url", &ver));

  try {
    Registry reg(Path::prefixRoot(Path::REGISTRY));
    reg.push(&ver);
    reg.commit();
  }
  catch(const reapack_error &e) {
    char msg[4096] = {};
    sprintf(msg,
      "ReaPack could not register itself! Please report this issue.\n\n"
      "Error description: %s", e.what());
    ShowMessageBox(msg, "ReaPack", MB_OK);
  }
}
