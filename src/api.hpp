/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2020  Christian Fillion
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

#ifndef REAPACK_API_HPP
#define REAPACK_API_HPP

#include <string>

struct APIFunc {
  const char *name;
  void *cImpl;
  void *reascriptImpl;
  void *definition;
};

class APIReg {
public:
  APIReg(const APIFunc *);
  APIReg(const APIReg &) = delete;
  ~APIReg();

private:
  void registerFunc() const;

  const APIFunc *m_func;

  // plugin_register requires these to not be temporaries
  std::string m_impl;
  std::string m_vararg;
  std::string m_help;
};

namespace API {
  // api_misc.cpp
  extern APIFunc BrowsePackages;
  extern APIFunc CompareVersions;
  extern APIFunc ProcessQueue;

  // api_package.cpp
  extern APIFunc AboutInstalledPackage;
  extern APIFunc EnumOwnedFiles;
  extern APIFunc FreeEntry;
  extern APIFunc GetEntryInfo;
  extern APIFunc GetOwner;

  // api_repo.cpp
  extern APIFunc AboutRepository;
  extern APIFunc AddSetRepository;
  extern APIFunc GetRepositoryInfo;
};

#endif
