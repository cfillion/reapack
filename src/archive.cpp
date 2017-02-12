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
#include "path.hpp"
#include "reapack.hpp"
#include "registry.hpp"

#include <fstream>

#include <WDL/zlib/zip.h>
#include <WDL/zlib/unzip.h>

using namespace std;

size_t Archive::write(const auto_string &path, ReaPack *reapack)
{
  size_t count = 0;

  Registry reg(Path::prefixRoot(Path::REGISTRY));
  ArchiveWriter writer(path);

  for(const Remote &remote : reapack->config()->remotes.getEnabled()) {
    for(const Registry::Entry &entry : reg.getEntries(remote.name())) {
      ++count;

      for(const Registry::File &file : reg.getFiles(entry))
        writer.addFile(file.path);
    }
  }

  return count;
}

ArchiveWriter::ArchiveWriter(const auto_string &path)
{
  m_zip = zipOpen(reinterpret_cast<const char *>(path.c_str()),
    APPEND_STATUS_CREATE);

  if(!m_zip)
    throw reapack_error(FS::lastError().c_str());
}

ArchiveWriter::~ArchiveWriter()
{
  zipClose(m_zip, nullptr);
}

void ArchiveWriter::addFile(const Path &path)
{
  ifstream stream(make_autostring(Path::prefixRoot(path).join()), ifstream::binary);
  if(!stream)
    throw reapack_error(FS::lastError().c_str());

  const int status = zipOpenNewFileInZip(m_zip, path.join().c_str(), nullptr,
    nullptr, 0, nullptr, 0, nullptr, Z_DEFLATED, Z_DEFAULT_COMPRESSION);

  if(status != ZIP_OK)
    throw reapack_error(errorMessage(status).c_str());

  string buffer(1024, 0);

  const auto readChunk = [&] {
    stream.read(&buffer[0], buffer.size());
    return (int)stream.gcount();
  };

  while(const int len = readChunk())
    zipWriteInFileInZip(m_zip, &buffer[0], len);

  zipCloseFileInZip(m_zip);
}

string ArchiveWriter::errorMessage(const int code)
{
  switch(code) {
  case ZIP_OK:
    return "No error";
  case ZIP_ERRNO:
    return FS::lastError();
  case ZIP_PARAMERROR:
    return "Invalid parameter";
  case ZIP_BADZIPFILE:
    return "Bad zip file";
  case ZIP_INTERNALERROR:
    return "Internal Zip Error";
  default:
    return "Unknown error";
  }
}
