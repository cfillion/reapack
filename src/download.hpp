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

#ifndef REAPACK_DOWNLOAD_HPP
#define REAPACK_DOWNLOAD_HPP

#include "config.hpp"
#include "thread.hpp"

#include <string>

#include <curl/curl.h>

struct DownloadContext {
  static void GlobalInit();
  static void GlobalCleanup();

  DownloadContext();
  ~DownloadContext();

  CURL *m_curl;
};

class Download : public ThreadTask {
public:
  enum Flag {
    NoCacheFlag = 1<<0,
  };

  Download(const std::string &name, const std::string &url,
    const NetworkOpts &, int flags = 0);

  std::string summary() const override;
  const std::string &url() const { return m_url; }
  const NetworkOpts &options() const { return m_opts; }
  bool has(Flag f) const { return (m_flags & f) != 0; }

  const std::string &contents() { return m_contents; }

  void start();
  void run(DownloadContext *) override;

private:
  static size_t WriteData(char *, size_t, size_t, void *);
  static int UpdateProgress(void *, double, double, double, double);

  std::string m_name;
  std::string m_url;
  NetworkOpts m_opts;
  int m_flags;
  std::string m_contents;
};

#endif
