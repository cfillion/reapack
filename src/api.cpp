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

#include "api.hpp"

#include <boost/preprocessor.hpp>
#include <cstdint>
#include <cstdio>

#include <reaper_plugin_functions.h>

#include "version.hpp"

using namespace std;

#define API_PREFIX "ReaPack_"

APIDef::APIDef(const APIFunc *func)
  : m_func(func)
{
  plugin_register(m_func->cKey, m_func->cImpl);
  plugin_register(m_func->reascriptKey, m_func->reascriptImpl);
  plugin_register(m_func->definitionKey, m_func->definition);
}

APIDef::~APIDef()
{
  unregister(m_func->cKey, m_func->cImpl);
  unregister(m_func->reascriptKey, m_func->reascriptImpl);
  unregister(m_func->definitionKey, m_func->definition);
}

void APIDef::unregister(const char *key, void *ptr)
{
  char buf[255];
  snprintf(buf, sizeof(buf), "-%s", key);
  plugin_register(buf, ptr);
}

#define ARG_TYPE(arg) BOOST_PP_TUPLE_ELEM(2, 0, arg)
#define ARG_NAME(arg) BOOST_PP_TUPLE_ELEM(2, 1, arg)

#define ARGS(r, data, i, arg) BOOST_PP_COMMA_IF(i) ARG_TYPE(arg) ARG_NAME(arg)
#define PARAMS(r, data, i, arg) BOOST_PP_COMMA_IF(i) (ARG_TYPE(arg))(intptr_t)argv[i]
#define DEFARGS(r, macro, i, arg) \
  BOOST_PP_IF(i, ",", BOOST_PP_EMPTY()) BOOST_PP_STRINGIZE(macro(arg))

#define DEFINE_API(type, name, args, help, ...) \
  namespace name { \
    static type cImpl(BOOST_PP_SEQ_FOR_EACH_I(ARGS, _, args)) __VA_ARGS__ \
    void *reascriptImpl(void **argv, int argc) { \
      if(argc == BOOST_PP_SEQ_SIZE(args)) \
        return (void *)(intptr_t)cImpl(BOOST_PP_SEQ_FOR_EACH_I(PARAMS, _, args)); \
      return nullptr; \
    } \
    static const char *definition = #type "\0" \
      BOOST_PP_SEQ_FOR_EACH_I(DEFARGS, ARG_TYPE, args) "\0" \
      BOOST_PP_SEQ_FOR_EACH_I(DEFARGS, ARG_NAME, args) "\0" help; \
  }; \
  APIFunc API::name = {\
    "API_" API_PREFIX #name, (void *)&name::cImpl, \
    "APIvararg_" API_PREFIX #name, (void *)&name::reascriptImpl, \
    "APIdef_" API_PREFIX #name, (void *)name::definition, \
  }

DEFINE_API(bool, CompareVersions, ((const char*, ver1))((const char*, ver2))
    ((int*, resultOut))((char*, errorOut))((int, errorOut_sz)),
  "Compare version numbers. Returns 0 if both versions are equal,"
  " a positive value if ver1 is higher than ver2"
  " and a negative value otherwise.",
{
  VersionName a, b;
  string error;

  if(a.tryParse(ver1, &error) && b.tryParse(ver2, &error)) {
    *resultOut = a.compare(b);
    return true;
  }

  if(errorOut)
    snprintf(errorOut, errorOut_sz, "%s", error.c_str());

  *resultOut = 0;

  return false;
});
