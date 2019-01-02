/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2019  Christian Fillion
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

#ifndef REAPACK_PLATFORM_HPP
#define REAPACK_PLATFORM_HPP

class Platform {
public:
  enum Enum {
    UnknownPlatform,
    GenericPlatform,

    WindowsPlatform,
    Win32Platform,
    Win64Platform,

    DarwinPlatform,
    Darwin32Platform,
    Darwin64Platform,

    LinuxPlatform,
    Linux32Platform,
    Linux64Platform,
  };

  Platform() : m_value(GenericPlatform) {}
  Platform(Enum val) : m_value(val) {}
  Platform(const char *str) : m_value(parse(str)) {}

  Enum value() const { return m_value; }
  bool test() const;

  operator Enum() const { return m_value; }
  Platform &operator=(Enum n) { m_value = n; return *this; }

private:
  static Enum parse(const char *);

  Enum m_value;
};

#endif
