/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2024  Christian Fillion
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
    Unknown,

    Darwin_i386   = 1<<0,
    Darwin_x86_64 = 1<<1,
    Darwin_arm64  = 1<<2,
    Darwin_Any    = Darwin_i386 | Darwin_x86_64 | Darwin_arm64,

    Linux_i686    = 1<<3,
    Linux_x86_64  = 1<<4,
    Linux_armv7l  = 1<<5,
    Linux_aarch64 = 1<<6,
    Linux_Any     = Linux_i686 | Linux_x86_64 | Linux_armv7l | Linux_aarch64,

    Windows_x86   = 1<<7,
    Windows_x64   = 1<<8,
    Windows_Any   = Windows_x86 | Windows_x64,

    Generic       = Darwin_Any | Linux_Any | Windows_Any,
  };

  static const Enum Current;

  Platform() : m_value(Generic) {}
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
