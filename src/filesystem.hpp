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

#ifndef REAPACK_FILESYSTEM_HPP
#define REAPACK_FILESYSTEM_HPP

#include <string>

class Path;

namespace FS {
  FILE *open(const Path &);
  bool write(const Path &, const std::string &);
  bool rename(const Path &, const Path &);
  bool remove(const Path &);
  bool removeRecursive(const Path &);
  bool mtime(const Path &, time_t *);

  std::string lastError();
};

#endif
