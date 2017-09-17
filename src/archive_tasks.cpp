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
#include "filesystem.hpp"
#include "index.hpp"
#include "reapack.hpp"
#include "remote.hpp"
#include "transaction.hpp"

#include <iomanip>
#include <sstream>

using namespace std;

static const Path ARCHIVE_TOC("toc");

ExportTask::ExportTask(const string &path, Transaction *tx)
  : Task(tx), m_path(path)
{
}

bool ExportTask::start()
{
  stringstream toc;
  ArchiveWriterPtr writer;

  try {
    writer = make_shared<ArchiveWriter>(m_path.temp());
  }
  catch(const reapack_error &e) {
    tx()->receipt()->addError({string("Could not open archive for writing: ") +
      e.what(), m_path.temp().join()});
    return false;
  }

  vector<FileCompressor *> jobs;

  for(const Remote &remote : g_reapack->config()->remotes.getEnabled()) {
    bool addedRemote = false;

    for(const Registry::Entry &entry : tx()->registry()->getEntries(remote.name())) {
      if(!addedRemote) {
        toc << "REPO " << remote.toString() << '\n';
        jobs.push_back(new FileCompressor(Index::pathFor(remote.name()), writer));
        addedRemote = true;
      }

      toc << "PACK "
        << quoted(entry.category) << '\x20'
        << quoted(entry.package) << '\x20'
        << quoted(entry.version.toString()) << '\x20'
        << entry.pinned << '\n'
      ;

      for(const Registry::File &file : tx()->registry()->getFiles(entry))
        jobs.push_back(new FileCompressor(file.path, writer));
    }
  }

  writer->addFile(ARCHIVE_TOC, toc);

  // Start after we've written the table of contents in the main thread
  // because we cannot safely write into the zip from more than one
  // thread at the same time.
  for(FileCompressor *job : jobs) {
    job->onFinish([=] {
    if(job->state() == ThreadTask::Success)
      tx()->receipt()->addExport(job->path());
    });

    tx()->threadPool()->push(job);
  }

  return true;
}

void ExportTask::commit()
{
  if(!FS::rename(m_path)) {
    tx()->receipt()->addError({string("Could not move to permanent location: ") +
      FS::lastError(), m_path.target().prependRoot().join()});
  }
}

void ExportTask::rollback()
{
  FS::remove(m_path.temp());
}
