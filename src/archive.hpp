/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2021  Christian Fillion
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

#ifndef REAPACK_ARCHIVE_HPP
#define REAPACK_ARCHIVE_HPP

#include "path.hpp"
#include "thread.hpp"

class ThreadPool;

typedef void *zipFile;

namespace Archive {
  void import(const std::string &path);
};

class ArchiveReader {
public:
  ArchiveReader(const Path &path);
  ~ArchiveReader();
  int extractFile(const Path &);
  int extractFile(const Path &, std::ostream &) noexcept;

private:
  zipFile m_zip;
};

typedef std::shared_ptr<ArchiveReader> ArchiveReaderPtr;

class ArchiveWriter {
public:
  ArchiveWriter(const Path &path);
  ~ArchiveWriter();
  int addFile(const Path &fn);
  int addFile(const Path &fn, std::istream &) noexcept;

private:
  zipFile m_zip;
};

typedef std::shared_ptr<ArchiveWriter> ArchiveWriterPtr;

class FileExtractor : public ThreadTask {
public:
  FileExtractor(const Path &target, const ArchiveReaderPtr &);
  const TempPath &path() const { return m_path; }

  bool concurrent() const override { return false; }
  bool run() override;

private:
  TempPath m_path;
  ArchiveReaderPtr m_reader;
};

class FileCompressor : public ThreadTask {
public:
  FileCompressor(const Path &target, const ArchiveWriterPtr &);
  const Path &path() const { return m_path; }

  bool concurrent() const override { return false; }
  bool run() override;

private:
  Path m_path;
  ArchiveWriterPtr m_writer;
};

#endif
