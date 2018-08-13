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
#include "path.hpp"
#include "thread.hpp"

#include <curl/curl.h>
#include <fstream>
#include <sstream>

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

class Download : public ThreadTask {
public:
  enum Flag {
    NoCacheFlag = 1<<0,
  };

  Download(const std::string &url, const NetworkOpts &, int flags = 0);

  void setName(const std::string &);
  const std::string &url() const { return m_url; }

  bool concurrent() const override { return true; }
  bool run() override;

protected:
  virtual std::ostream *openStream() = 0;
  virtual void closeStream() {}

private:
  bool has(Flag f) const { return (m_flags & f) != 0; }
  static size_t WriteData(char *, size_t, size_t, void *);
  static int UpdateProgress(void *, double, double, double, double);

  std::string m_url;
  NetworkOpts m_opts;
  int m_flags;
};

class MemoryDownload : public Download {
public:
  MemoryDownload(const std::string &url, const NetworkOpts &, int flags = 0);

  std::string contents() const { return m_stream.str(); }

protected:
  std::ostream *openStream() override { return &m_stream; }

private:
  std::stringstream m_stream;
};

class FileDownload : public Download {
public:
  FileDownload(const Path &target, const std::string &url,
    const NetworkOpts &, int flags = 0);

  const TempPath &path() const { return m_path; }
  bool save();

protected:
  std::ostream *openStream() override;
  void closeStream() override;

private:
  TempPath m_path;
  std::ofstream m_stream;
};

#endif
