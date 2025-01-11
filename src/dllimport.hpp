/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2025  Christian Fillion
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

#ifndef REAPACK_DLLIMPORT_HPP
#define REAPACK_DLLIMPORT_HPP

#ifndef _WIN32
#  error This file should not be included on non-Windows systems.
#endif

#include <windows.h>

template<typename Proc, typename = std::enable_if_t<std::is_function_v<Proc>>>
class DllImport {
public:
  DllImport(const wchar_t *dll, const char *func)
  {
    if (m_lib = LoadLibrary(dll))
      m_proc = reinterpret_cast<Proc *>(GetProcAddress(m_lib, func));
    else
      m_proc = nullptr;
  }

  ~DllImport()
  {
    if (m_lib)
      FreeLibrary(m_lib);
  }

  operator bool() const { return m_proc != nullptr; }

  template<typename... Args>
  auto operator()(Args&&... args) const { return m_proc(std::forward<Args>(args)...); }

private:
  HINSTANCE m_lib;
  Proc *m_proc;
};

#endif
