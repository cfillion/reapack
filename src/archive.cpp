/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2025  Christian Fillion
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

#include "archive.hpp"

#include "config.hpp"
#include "errors.hpp"
#include "filesystem.hpp"
#include "index.hpp"
#include "path.hpp"
#include "reapack.hpp"
#include "transaction.hpp"
#include "win32.hpp"
#include <WDL/localize/localize.h>

#include <fstream>
#include <iomanip>
#include <sstream>

#include <zip.h>
#include <unzip.h>
#include <ioapi.h>

static const Path ARCHIVE_TOC("toc");
static const size_t BUFFER_SIZE = 4096;

#ifdef _WIN32
static void *wide_fopen(voidpf, const void *filename, int mode)
{
  const wchar_t *fopen_mode = nullptr;

  if((mode & ZLIB_FILEFUNC_MODE_READWRITEFILTER) == ZLIB_FILEFUNC_MODE_READ)
    fopen_mode = L"rb";
  else if(mode & ZLIB_FILEFUNC_MODE_EXISTING)
    fopen_mode = L"r+b";
  else if(mode & ZLIB_FILEFUNC_MODE_CREATE)
    fopen_mode = L"wb";

  FILE *file = nullptr;

  if(filename && fopen_mode) {
    const auto &&wideFilename = Win32::widen(static_cast<const char *>(filename));
    _wfopen_s(&file, wideFilename.c_str(), fopen_mode);
  }

  return file;
}
#endif

struct ImportArchive {
  void importRemote(const std::string &);
  void importPackage(const std::string &);

  ArchiveReaderPtr m_reader;
  RemoteList *m_remotes;
  Transaction *m_tx;
  IndexPtr m_lastIndex;
};

void Archive::import(const std::string &path)
{
  ImportArchive state{std::make_shared<ArchiveReader>(path),
    &g_reapack->config()->remotes};

  std::stringstream toc;
  if(const int err = state.m_reader->extractFile(ARCHIVE_TOC, toc))
    throw reapack_error(String::format(
      __LOCALIZE("Cannot locate the table of contents (%d)", "reapack_error_msg"), err));

  // starting import, do not abort process (eg. by throwing) at this point
  if(!(state.m_tx = g_reapack->setupTransaction()))
    return;

  std::string line;
  while(std::getline(toc, line)) {
    if(line.size() <= 5) // 5 is the length of the line type prefix
      continue;

    const std::string &data = line.substr(5);

    try {
      switch(line[0]) {
      case 'R':
        state.importRemote(data);
        break;
      case 'P':
        state.importPackage(data);
        break;
      default:
        throw reapack_error(String::format(__LOCALIZE("Unknown token '%s' (skipping)", "reapack_error_msg"),
          line.substr(0, 4).c_str()));
      }
    }
    catch(const reapack_error &e) {
      state.m_tx->receipt()->addError({e.what(), path});
    }
  }

  g_reapack->config()->write();
  state.m_tx->runTasks();
}

void ImportArchive::importRemote(const std::string &data)
{
  m_lastIndex = nullptr; // clear the previous repository
  Remote remote = Remote::fromString(data);

  if(const int err = m_reader->extractFile(Index::pathFor(remote.name()))) {
    throw reapack_error(String::format(__LOCALIZE("Failed to extract index of %s (%d)", "reapack_error_msg"),
      remote.name().c_str(), err));
  }

  const Remote &original = m_remotes->get(remote.name());
  if(original.isProtected()) {
    remote.setUrl(original.url());
    remote.protect();
  }

  m_remotes->add(remote);
  m_lastIndex = Index::load(remote.name());
}

void ImportArchive::importPackage(const std::string &data)
{
  // don't report an error if the index isn't loaded assuming we already
  // did when failing to import the repository above
  if(!m_lastIndex)
    return;

  std::string categoryName, packageName, versionName;
  int flags;

  std::istringstream stream(data);
  stream
    >> quoted(categoryName) >> quoted(packageName) >> quoted(versionName)
    >> flags;

  const Package *pkg = m_lastIndex->find(categoryName, packageName);
  const Version *ver = pkg ? pkg->findVersion(versionName) : nullptr;

  if(!ver) {
    throw reapack_error(String::format(
      __LOCALIZE("%s/%s/%s v%s cannot be found or is incompatible with your operating system.", "reapack_error_msg"),
      m_lastIndex->name().c_str(), categoryName.c_str(),
      packageName.c_str(), versionName.c_str()));
  }

  m_tx->install(ver, flags, m_reader);
}

ArchiveReader::ArchiveReader(const Path &path)
{
  zlib_filefunc64_def filefunc;
  fill_fopen64_filefunc(&filefunc);
#ifdef _WIN32
  filefunc.zopen64_file = wide_fopen;
#endif

  m_zip = unzOpen2_64(path.join().c_str(), &filefunc);

  if(!m_zip)
    throw reapack_error(FS::lastError());
}

ArchiveReader::~ArchiveReader()
{
  unzClose(m_zip);
}

int ArchiveReader::extractFile(const Path &path)
{
  std::ofstream stream;

  if(FS::open(stream, path))
    return extractFile(path, stream);
  else {
    throw reapack_error(String::format("%s: %s",
      path.join().c_str(), FS::lastError()));
  }
}

int ArchiveReader::extractFile(const Path &path, std::ostream &stream) noexcept
{
  int status = unzLocateFile(m_zip, path.join(false).c_str(), false);
  if(status != UNZ_OK)
    return status;

  status = unzOpenCurrentFile(m_zip);
  if(status != UNZ_OK)
    return status;

  std::string buffer(BUFFER_SIZE, 0);

  const auto readChunk = [&] {
    return unzReadCurrentFile(m_zip, &buffer[0], static_cast<int>(buffer.size()));
  };

  while(const int len = readChunk()) {
    if(len < 0)
      return len; // read error

    stream.write(&buffer[0], len);
  }

  return unzCloseCurrentFile(m_zip);
}

FileExtractor::FileExtractor(const Path &target, const ArchiveReaderPtr &reader)
  : m_path(target), m_reader(reader)
{
  setSummary({ "Extracting", target.join() });
}

bool FileExtractor::run()
{
  std::ofstream stream;
  if(!FS::open(stream, m_path.temp())) {
    setError({FS::lastError(), m_path.temp().join()});
    return false;
  }

  const int error = m_reader->extractFile(m_path.target(), stream);
  stream.close();

  if(error) {
    setError({String::format(__LOCALIZE("Failed to extract file (%d)", "reapack_error_msg"), error),
      m_path.target().join()});
    return false;
  }

  return true;
}

ArchiveWriter::ArchiveWriter(const Path &path)
{
  zlib_filefunc64_def filefunc;
  fill_fopen64_filefunc(&filefunc);
#ifdef _WIN32
  filefunc.zopen64_file = wide_fopen;
#endif

  m_zip = zipOpen2_64(path.join().c_str(), APPEND_STATUS_CREATE, nullptr, &filefunc);

  if(!m_zip)
    throw reapack_error(FS::lastError());
}

ArchiveWriter::~ArchiveWriter()
{
  zipClose(m_zip, nullptr);
}

int ArchiveWriter::addFile(const Path &path)
{
  std::ifstream stream;

  if(FS::open(stream, path))
    return addFile(path, stream);
  else {
    throw reapack_error(String::format("%s: %s",
      path.join().c_str(), FS::lastError()));
  }
}

int ArchiveWriter::addFile(const Path &path, std::istream &stream) noexcept
{
  const int status = zipOpenNewFileInZip(m_zip, path.join(false).c_str(), nullptr,
    nullptr, 0, nullptr, 0, nullptr, Z_DEFLATED, Z_DEFAULT_COMPRESSION);

  if(status != ZIP_OK)
    return status;

  std::string buffer(BUFFER_SIZE, 0);

  const auto readChunk = [&] {
    stream.read(&buffer[0], buffer.size());
    return static_cast<int>(stream.gcount());
  };

  while(const int len = readChunk()) {
    if(len < 0)
      return len; // write error

    zipWriteInFileInZip(m_zip, &buffer[0], len);
  }

  return zipCloseFileInZip(m_zip);
}

FileCompressor::FileCompressor(const Path &target, const ArchiveWriterPtr &writer)
  : m_path(target), m_writer(writer)
{
  setSummary({ __LOCALIZE("Compressing", "reapack_archive"), target.join() });
}

bool FileCompressor::run()
{
  std::ifstream stream;
  if(!FS::open(stream, m_path)) {
    setError({
      String::format(__LOCALIZE("Could not open file for export (%s)", "reapack_error_msg"), FS::lastError()),
      m_path.join()});
    return false;
  }

  const int error = m_writer->addFile(m_path, stream);
  stream.close();

  if(error) {
    setError({String::format(__LOCALIZE("Failed to compress file (%d)", "reapack_error_msg"), error), m_path.join()});
    return false;
  }

  return true;
}
