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

#ifndef REAPACK_API_HELPER_HPP
#define REAPACK_API_HELPER_HPP

#include <boost/mpl/aux_/preprocessor/token_equal.hpp>
#include <boost/preprocessor.hpp>

#define API_PREFIX "ReaPack_"

#define BOOST_MPL_PP_TOKEN_EQUAL_void(x) x
#define BOOST_MPL_PP_TOKEN_EQUAL_bool(x) x
#define BOOST_MPL_PP_TOKEN_EQUAL_int(x) x
#define IS_VOID(type) BOOST_MPL_PP_TOKEN_EQUAL(type, void)
#define IS_BOOL(type) BOOST_MPL_PP_TOKEN_EQUAL(type, bool)
#define IS_INT(type)  BOOST_MPL_PP_TOKEN_EQUAL(type, int)

#define ARG_TYPE(arg) BOOST_PP_TUPLE_ELEM(2, 0, arg)
#define ARG_NAME(arg) BOOST_PP_TUPLE_ELEM(2, 1, arg)

// produce an appropriate conversion from void* to any type
#define VOIDP_TO(type, val) \
  BOOST_PP_IF(IS_BOOL(type), val != 0, \
    (type) BOOST_PP_EXPR_IF(IS_INT(type), (intptr_t)) val)

#define DEFARGS(r, data, i, arg) BOOST_PP_COMMA_IF(i) ARG_TYPE(arg) ARG_NAME(arg)
#define CALLARGS(r, data, i, arg) \
  BOOST_PP_COMMA_IF(i) VOIDP_TO(ARG_TYPE(arg), argv[i])
#define DOCARGS(r, macro, i, arg) \
  BOOST_PP_EXPR_IF(i, ",") BOOST_PP_STRINGIZE(macro(arg))

#define DEFINE_API(type, name, args, help, ...) \
  namespace API_##name { \
    static type cImpl(BOOST_PP_SEQ_FOR_EACH_I(DEFARGS, _, args)) __VA_ARGS__ \
    static void *reascriptImpl(void **argv, int argc) { \
      BOOST_PP_EXPR_IF(BOOST_PP_NOT(IS_VOID(type)), return (void *)(intptr_t)) \
      cImpl(BOOST_PP_SEQ_FOR_EACH_I(CALLARGS, _, args)); \
      BOOST_PP_EXPR_IF(IS_VOID(type), return nullptr;) \
    } \
    static const char *definition = #type "\0" \
      BOOST_PP_SEQ_FOR_EACH_I(DOCARGS, ARG_TYPE, args) "\0" \
      BOOST_PP_SEQ_FOR_EACH_I(DOCARGS, ARG_NAME, args) "\0" help; \
  }; \
  APIFunc API::name = {\
    "API_" API_PREFIX #name, (void *)&API_##name::cImpl, \
    "APIvararg_" API_PREFIX #name, (void *)&API_##name::reascriptImpl, \
    "APIdef_" API_PREFIX #name, (void *)API_##name::definition, \
  }

#endif
