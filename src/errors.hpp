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

#ifndef REAPACK_ERRORS_HPP
#define REAPACK_ERRORS_HPP

#include <stdexcept>
#include <string>

class reapack_error : public std::runtime_error {
public:
  using runtime_error::runtime_error;

  template<typename... Args> reapack_error(const char *fmt, Args&&... args)
    : runtime_error(format(fmt, std::forward<Args>(args)...)) {}

private:
  template<typename... Args> std::string format(const char *fmt, Args&&... args)
  {
    const int size = snprintf(nullptr, 0, fmt, args...);
    std::string buf(size, 0);
    snprintf(&buf[0], size + 1, fmt, args...);
    return buf;
  }
};

struct ErrorInfo {
  std::string message;
  std::string context;
};

#endif
