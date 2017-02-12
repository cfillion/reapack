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

#include "archive.hpp"

#include "config.hpp"
#include "errors.hpp"
#include "filesystem.hpp"
#include "index.hpp"
#include "path.hpp"
#include "reapack.hpp"
#include "registry.hpp"

#include <fstream>
#include <sstream>

#include <zlib/zip.h>
#include <zlib/unzip.h>
#include <zlib/ioapi.h>

using namespace std;

static const size_t BUFFER_SIZE = 4096;

#ifdef _WIN32
static void *wide_fopen(voidpf opaque, const void *filename, int mode)
{
  const wchar_t *fopen_mode = nullptr;

  if((mode & ZLIB_FILEFUNC_MODE_READWRITEFILTER) == ZLIB_FILEFUNC_MODE_READ)
    fopen_mode = L"rb";
  else if(mode & ZLIB_FILEFUNC_MODE_EXISTING)
    fopen_mode = L"r+b";
  else if(mode & ZLIB_FILEFUNC_MODE_CREATE)
    fopen_mode = L"wb";

  FILE *file = nullptr;

  if(filename && fopen_mode)
    _wfopen_s(&file, static_cast<const wchar_t *>(filename), fopen_mode);

  return file;
}
#endif

size_t Archive::write(const auto_string &path, ReaPack *reapack)
{
  size_t count = 0;

  ArchiveWriter writer(path);
  writer.addFile(Path::REGISTRY);

  stringstream remotes;
  Registry reg(Path::prefixRoot(Path::REGISTRY));

  for(const Remote &remote : reapack->config()->remotes.getEnabled()) {
    bool hasPackages = false;

    for(const Registry::Entry &entry : reg.getEntries(remote.name())) {
      hasPackages = true;
      ++count;

      for(const Registry::File &file : reg.getFiles(entry)) {
        if(writer.addFile(file.path) != ZIP_OK)
          ; // TODO: report failure
      }
    }

    if(hasPackages) {
      remotes << remote.toString() << "\n";
      writer.addFile(Index::pathFor(remote.name()));
    }
  }

  writer.addFile(Path("ReaPack/remotes"), remotes);

  return count;
}

ArchiveWriter::ArchiveWriter(const auto_string &path)
{
  zlib_filefunc64_def filefunc{};
  fill_fopen64_filefunc(&filefunc);
#ifdef _WIN32
  filefunc.zopen64_file = wide_fopen;
#endif

  m_zip = zipOpen2_64(reinterpret_cast<const char *>(path.c_str()),
    APPEND_STATUS_CREATE, nullptr, &filefunc);

  if(!m_zip)
    throw reapack_error(FS::lastError().c_str());
}

ArchiveWriter::~ArchiveWriter()
{
  zipClose(m_zip, nullptr);
}

int ArchiveWriter::addFile(const Path &path)
{
  ifstream stream(make_autostring(Path::prefixRoot(path).join()), ifstream::binary);

  if(stream)
    return addFile(path, stream);
  else
    return Z_ERRNO;
}

int ArchiveWriter::addFile(const Path &path, istream &stream)
{

  const int status = zipOpenNewFileInZip(m_zip, path.join('/').c_str(), nullptr,
    nullptr, 0, nullptr, 0, nullptr, Z_DEFLATED, Z_DEFAULT_COMPRESSION);

  if(status != ZIP_OK)
    return status;

  string buffer(BUFFER_SIZE, 0);

  const auto readChunk = [&] {
    stream.read(&buffer[0], buffer.size());
    return (int)stream.gcount();
  };

  while(const int len = readChunk())
    zipWriteInFileInZip(m_zip, &buffer[0], len);

  return zipCloseFileInZip(m_zip);
}
