/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2023  Christian Fillion
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

#ifndef REAPACK_STRING_HPP
#define REAPACK_STRING_HPP

#include <sstream>
#include <string>

namespace String {
  namespace ImplDetail {
    void imbueStream(std::ostream &);
  };

#ifdef __GNUC__
  __attribute__((format(printf, 1, 2)))
#endif
  std::string format(const char *fmt, ...);

  std::string indent(const std::string &);
  std::string stripRtf(const std::string &);

  template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
  std::string number(const T v)
  {
    std::ostringstream stream;
    ImplDetail::imbueStream(stream);
    stream << v;
    return stream.str();
  };
}

#endif
