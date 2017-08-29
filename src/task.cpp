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

#include "task.hpp"

#include "archive.hpp"
#include "config.hpp"
#include "download.hpp"
#include "errors.hpp"
#include "filesystem.hpp"
#include "index.hpp"
#include "reapack.hpp"
#include "transaction.hpp"

using namespace std;

Task::Task(Transaction *tx) : m_tx(tx)
{
}

InstallTask::InstallTask(const Version *ver, const bool pin,
    const Registry::Entry &re, const ArchiveReaderPtr &reader, Transaction *tx)
  : Task(tx), m_version(ver), m_pin(pin), m_oldEntry(move(re)), m_reader(reader),
    m_fail(false), m_index(ver->package()->category()->index()->shared_from_this())
{
}

bool InstallTask::start()
{
  // get current files before overwriting the entry
  m_oldFiles = tx()->registry()->getFiles(m_oldEntry);

  // prevent file conflicts (don't worry, the registry push is reverted)
  try {
    vector<Path> conflicts;
    tx()->registry()->push(m_version, &conflicts);

    if(!conflicts.empty()) {
      for(const Path &path : conflicts) {
        tx()->receipt()->addError({L"Conflict: " + path.join() +
          L" is already owned by another package", m_version->fullName()});
      }

      return false;
    }
  }
  catch(const reapack_error &e) {
    tx()->receipt()->addError({e.what(), m_version->fullName()});
    return false;
  }

  for(const Source *src : m_version->sources()) {
    const Path &targetPath = src->targetPath();

    const auto &old = find_if(m_oldFiles.begin(), m_oldFiles.end(),
      [&](const Registry::File &f) { return f.path == targetPath; });

    if(old != m_oldFiles.end())
      m_oldFiles.erase(old);

    if(m_reader) {
      FileExtractor *ex = new FileExtractor(targetPath, m_reader);
      push(ex, ex->path());
    }
    else {
      const NetworkOpts &opts = g_reapack->config()->network;
      FileDownload *dl = new FileDownload(targetPath, src->url(), opts);
      push(dl, dl->path());
    }
  }

  return true;
}

void InstallTask::push(ThreadTask *job, const TempPath &path)
{
  job->onStart([=] { m_newFiles.push_back(path); });
  job->onFinish([=] {
    m_waiting.erase(job);

    if(job->state() != ThreadTask::Success)
      rollback();
  });

  m_waiting.insert(job);
  tx()->threadPool()->push(job);
}

void InstallTask::commit()
{
  if(m_fail)
    return;

  for(const TempPath &paths : m_newFiles) {
    if(!FS::rename(paths)) {
      tx()->receipt()->addError({L"Cannot rename to target: " + FS::lastError(),
        paths.target().join()});

      // it's a bit late to rollback here as some files might already have been
      // overwritten. at least we can delete the temporary files
      rollback();
      return;
    }
  }

  for(const Registry::File &file : m_oldFiles) {
    if(FS::remove(file.path))
      tx()->receipt()->addRemoval(file.path);

    tx()->registerFile({false, m_oldEntry, file});
  }

  InstallTicket::Type type;

  if(m_oldEntry && m_oldEntry.version < m_version->name())
    type = InstallTicket::Upgrade;
  else
    type = InstallTicket::Install;

  tx()->receipt()->addTicket({type, m_version, m_oldEntry});

  const Registry::Entry newEntry = tx()->registry()->push(m_version);

  if(newEntry.type == Package::ExtensionType)
    tx()->receipt()->setRestartNeeded(true);

  if(m_pin)
    tx()->registry()->setPinned(newEntry, true);

  tx()->registerAll(true, newEntry);
}

void InstallTask::rollback()
{
  for(const TempPath &paths : m_newFiles)
    FS::removeRecursive(paths.temp());

  for(ThreadTask *job : m_waiting)
    job->abort();

  m_fail = true;
}

UninstallTask::UninstallTask(const Registry::Entry &re, Transaction *tx)
  : Task(tx), m_entry(move(re))
{
}

bool UninstallTask::start()
{
  tx()->registry()->getFiles(m_entry).swap(m_files);

  // allow conflicting packages to be installed
  tx()->registry()->forget(m_entry);

  return true;
}

void UninstallTask::commit()
{
  for(const auto &file : m_files) {
    if(!FS::exists(file.path))
      continue;

    if(FS::removeRecursive(file.path))
      tx()->receipt()->addRemoval(file.path);
    else
      tx()->receipt()->addError({FS::lastError(), file.path.join()});

    tx()->registerFile({false, m_entry, file});
  }

  tx()->registry()->forget(m_entry);
}

PinTask::PinTask(const Registry::Entry &re, const bool pin, Transaction *tx)
  : Task(tx), m_entry(move(re)), m_pin(pin)
{
}

void PinTask::commit()
{
  tx()->registry()->setPinned(m_entry, m_pin);
}
