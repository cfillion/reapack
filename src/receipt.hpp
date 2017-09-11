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

#include "errors.hpp"
#include "registry.hpp"

#include <set>
#include <string>
#include <unordered_set>
#include <vector>

class Index;
class Path;

typedef std::shared_ptr<const Index> IndexPtr;

struct InstallTicket {
  const Version *version;
  Registry::Entry previous;
};

class Receipt {
public:
  Receipt();

  bool empty() const;

  bool isRestartNeeded() const { return m_needRestart; }
  void setRestartNeeded(bool newVal) { m_needRestart = newVal; }

  void addInstall(const InstallTicket &);
  const std::vector<InstallTicket> &installs() const { return m_installs; }
  const std::vector<InstallTicket> &updates() const { return m_updates; }

  void addRemoval(const Path &p) { m_removals.insert(p); }
  void addRemovals(const std::set<Path> &);
  const std::set<Path> &removals() const { return m_removals; }

  void addExport(const Path &p) { m_exports.insert(p); }
  const std::set<Path> &exports() const { return m_exports; }

  void addError(const ErrorInfo &err) { m_errors.push_back(err); }
  const std::vector<ErrorInfo> &errors() const { return m_errors; }
  bool hasErrors() const { return !m_errors.empty(); }

private:
  bool m_enabled;
  bool m_needRestart;

  std::vector<InstallTicket> m_installs;
  std::vector<InstallTicket> m_updates;
  std::set<Path> m_removals;
  std::set<Path> m_exports;
  std::vector<ErrorInfo> m_errors;

  std::unordered_set<IndexPtr> m_indexes; // keep them alive!
};

#endif
