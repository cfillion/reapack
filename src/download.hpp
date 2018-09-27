/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2018  Christian Fillion
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

#ifndef REAPACK_DOWNLOAD_HPP
#define REAPACK_DOWNLOAD_HPP

#include "config.hpp"
#include "tempfile.hpp"
#include "thread.hpp"

#include <optional>
#include <sstream>

typedef void CURL;

class DownloadContext {
public:
  static void GlobalInit();
  static void GlobalCleanup();

  DownloadContext();
  ~DownloadContext();

  operator CURL*() { return m_curl; }

private:
  CURL *m_curl;
};

class Download : public WorkUnit {
public:
  enum Flag {
    NoCacheFlag = 1<<0,
  };

  Download(const std::string &url, const NetworkOpts &, int flags = 0);

  const std::string &url() const { return m_url; }
  bool failed() const { return m_error.has_value(); }
  const std::string &errorMessage() const { return m_error.value(); }

  bool run() override;

protected:
  virtual std::ostream &openStream() = 0;
  virtual void closeStream() {}

  std::optional<std::string> m_error;

private:
  bool has(Flag f) const { return (m_flags & f) != 0; }

  std::string m_url;
  NetworkOpts m_opts;
  int m_flags;
};

class MemoryDownload : public Download {
public:
  MemoryDownload(const std::string &url, const NetworkOpts &, int flags = 0);

  std::string contents() const { return m_stream.str(); }

protected:
  std::ostream &openStream() override { return m_stream; }

private:
  std::stringstream m_stream;
};

class FileDownload : public Download {
public:
  FileDownload(const Path &target, const std::string &url,
    const NetworkOpts &, int flags = 0);

  bool save() { return m_file.save(); }

protected:
  std::ostream &openStream() override;
  void closeStream() override;

private:
  TempFile m_file;
};

#endif
