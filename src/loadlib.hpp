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

#ifndef REAPACK_LOADLIB_HPP
#define REAPACK_LOADLIB_HPP

#ifdef _WIN32
#  include <windows.h>
#else
#  include <dlfcn.h>
#endif

#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>

class LoadLib {
public:
  static constexpr bool Optional {false};

  static bool loaded() { return g_ok; }

  template<typename... SoNames>
  LoadLib(SoNames... sonames)
    : LoadLib {true, sonames...}
  {}

  template<typename... SoNames>
  LoadLib(const bool required, SoNames... sonames)
    : m_required {required}
  {
    static_assert(sizeof...(sonames) > 0);

#ifdef _WIN32
    assert(!required && "error reporting not implemented");
    for(const wchar_t *soname : {sonames...}) {
      if((m_so = LoadLibrary(soname)))
#else
    for(const char *soname : {sonames...}) {
      if((m_so = dlopen(soname, RTLD_LAZY)))
#endif
        return;
    }

    fail();
  }

  ~LoadLib()
  {
    if(m_so)
#ifdef _WIN32
      FreeLibrary(m_so);
#else
      dlclose(m_so);
#endif
  }

  template<typename T>
  T get(const char *name)
  {
    if(!m_so)
      return nullptr;

    if(const T func {reinterpret_cast<T>(
#ifdef _WIN32
        GetProcAddress(m_so, func)
#else
        dlsym(m_so, name)
#endif
        )})
      return func;

    fail();
    return nullptr;
  }

private:
  void fail()
  {
    if(!m_required)
      return;

#ifndef _WIN32
    fprintf(stderr, "reapack: %s\n", dlerror());
#endif
    g_ok = false;
  }

  inline static bool g_ok { true };

#ifdef _WIN32
  HINSTANCE m_so;
#else
  void *m_so;
#endif
  bool m_required;
};

#ifdef LOADLIB_ENABLE
#define _LOADLIB_IMPORT(r, ns, name) \
  static auto name { g_##ns.get<decltype(&::name)>(BOOST_PP_STRINGIZE(name)) };
#define LOADLIB(ns, names, ...) \
  static LoadLib g_##ns names;  \
  namespace { namespace ns { \
    BOOST_PP_SEQ_FOR_EACH(_LOADLIB_IMPORT, ns, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)) \
  }}
#else
#  define _LOADLIB_IMPORT(r, ns, name) static auto name { &::name };
#define LOADLIB(ns, names, ...) \
  namespace { namespace ns { \
    BOOST_PP_SEQ_FOR_EACH(_LOADLIB_IMPORT, ns, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)) \
  }}
#endif

#endif
