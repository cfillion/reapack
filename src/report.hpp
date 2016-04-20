/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2016  Christian Fillion
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

#ifndef REAPACK_REPORT_HPP
#define REAPACK_REPORT_HPP

#include "dialog.hpp"

#include "registry.hpp"

#include <sstream>

class Package;
class Receipt;
class Version;

class ReportDialog : public Dialog {
public:
  ReportDialog();

protected:
  void onInit() override;

  virtual void fillReport() = 0;

  static const char * const NL;
  std::ostringstream &stream() { return m_stream; }

  void printHeader(const char *);
  void printVersion(const Version *);
  void printChangelog(const Version *);
  void printIndented(const std::string &);

private:
  std::ostringstream m_stream;
};

class Report : public ReportDialog {
public:
  Report(const Receipt *);

protected:
  void fillReport() override;

private:
  void printInstalls();
  void printUpdates();
  void printErrors();
  void printRemovals();

  const Receipt *m_receipt;
};

class History : public ReportDialog {
public:
  History(const Package *);

protected:
  void fillReport() override;

private:
  const Package *m_package;
};

class Contents : public ReportDialog {
public:
  Contents(const Package *);

protected:
  void fillReport() override;

private:
  const Package *m_package;
};

#endif
