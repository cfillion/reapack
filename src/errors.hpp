/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2022  Christian Fillion
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

#ifndef REAPACK_ERRORS_HPP
#define REAPACK_ERRORS_HPP

#include <ostream>
#include <stdexcept>

#include "string.hpp"

class reapack_error : public std::runtime_error {
public:
  using runtime_error::runtime_error;
};

struct ErrorInfo {
  std::string message;
  std::string context;
};

inline std::ostream &operator<<(std::ostream &os, const ErrorInfo &err)
{
  os << err.context << ":\r\n" << String::indent(err.message) << "\r\n";
  return os;
}

#endif
