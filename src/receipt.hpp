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

#ifndef REAPACK_RECEIPT_HPP
#define REAPACK_RECEIPT_HPP

#include "registry.hpp"
#include "errors.hpp"

#include <memory>
#include <set>
#include <sstream>
#include <vector>

class Index;
class InstallTicket;
class Path;
class ReceiptPage;
class Version;

typedef std::shared_ptr<const Index> IndexPtr;

class Receipt {
public:
  enum Flag {
    NoFlag             = 0,
    ErrorFlag          = 1<<0,
    RestartNeededFlag  = 1<<1,
    IndexChangedFlag   = 1<<2,
    PackageChangedFlag = 1<<3,
    InstalledFlag      = 1<<4,
    RemovedFlag        = 1<<5,
    ExportedFlag       = 1<<6,

    InstalledOrRemoved = InstalledFlag | RemovedFlag,
    RefreshBrowser = IndexChangedFlag | PackageChangedFlag | InstalledOrRemoved,
  };

  Receipt();

  int flags() const { return m_flags; }
  bool test(Flag f) const { return (m_flags & f) != 0; }
  bool empty() const;

  void setIndexChanged() { m_flags |= IndexChangedFlag; }
  void setPackageChanged() { m_flags |= PackageChangedFlag; }
  void addInstall(const Version *, const Registry::Entry &);
  void addRemoval(const Path &p);
  void addExport(const Path &p);
  void addError(const ErrorInfo &);

  ReceiptPage installedPage() const;
  ReceiptPage removedPage() const;
  ReceiptPage exportedPage() const;
  ReceiptPage errorPage() const;

private:
  int m_flags;

  std::multiset<InstallTicket> m_installs;
  std::set<Path> m_removals;
  std::set<Path> m_exports;
  std::vector<ErrorInfo> m_errors;
};

class ReceiptPage {
public:
  template<typename T>
  ReceiptPage(const T &list, const char *singular, const char *plural = nullptr)
    : m_size(list.size())
  {
    setTitle(m_size == 1 || !plural ? singular : plural);

    std::ostringstream stream;

    for(const auto &item : list) {
      if(stream.tellp() > 0)
        stream << "\r\n";

      stream << item;
    }

    m_contents = stream.str();
  }

  const std::string &title() const { return m_title; }
  const std::string &contents() const { return m_contents; }
  bool empty() const { return m_size == 0; }

private:
  void setTitle(const char *title);
  size_t m_size;
  std::string m_title;
  std::string m_contents;
};

class InstallTicket {
public:
  InstallTicket(const Version *ver, const Registry::Entry &previousEntry);

  bool operator<(const InstallTicket &) const;

private:
  friend std::ostream &operator<<(std::ostream &, const InstallTicket &);

  const Version *m_version;
  VersionName m_previous;
  bool m_isUpdate;

  IndexPtr m_index; // to keep it alive long enough
};

std::ostream &operator<<(std::ostream &, const InstallTicket &);

#endif
