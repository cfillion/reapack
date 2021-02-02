/* ReaPack: Package manager for REAPER
 * Copyright (C) 2015-2021  Christian Fillion
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

#include <tuple>

#include <boost/preprocessor.hpp>

template<typename T>
struct ReaScriptAPI;

template<typename R, typename... Args>
struct ReaScriptAPI<R(*)(Args...)>
{
  static void *applyVarArg(R(*fn)(Args...), void **argv, int argc)
  {
    if(static_cast<size_t>(argc) < sizeof...(Args))
      return nullptr;

    const auto &args = makeTuple(argv, std::index_sequence_for<Args...>{});

    if constexpr (std::is_void_v<R>) {
      std::apply(fn, args);
      return nullptr;
    }
    else {
      // cast integers to have the same size as a pointer to avoid warnings
      using IntPtrR = std::conditional_t<std::is_integral_v<R>, intptr_t, R>;
      const auto value = static_cast<IntPtrR>(std::apply(fn, args));
      return reinterpret_cast<void *>(value);
    }
  }

private:
  template<size_t I>
  using NthType = typename std::tuple_element<I, std::tuple<Args...>>::type;

  template<size_t... I>
  static auto makeTuple(void **argv, std::index_sequence<I...>)
  {
    return std::make_tuple((NthType<I>)reinterpret_cast<intptr_t>(argv[I])...);
  }
};

template<auto fn>
void *InvokeReaScriptAPI(void **argv, int argc)
{
  return ReaScriptAPI<decltype(fn)>::applyVarArg(fn, argv, argc);
}

#define ARG_TYPE(arg) BOOST_PP_TUPLE_ELEM(2, 0, arg)
#define ARG_NAME(arg) BOOST_PP_TUPLE_ELEM(2, 1, arg)

#define DEFARGS(r, data, i, arg) BOOST_PP_COMMA_IF(i) ARG_TYPE(arg) ARG_NAME(arg)
#define DOCARGS(r, macro, i, arg) \
  BOOST_PP_EXPR_IF(i, ",") BOOST_PP_STRINGIZE(macro(arg))

#define DEFINE_API(type, name, args, help, ...)                                 \
  static type API_##name(BOOST_PP_SEQ_FOR_EACH_I(DEFARGS, _, args)) __VA_ARGS__ \
                                                                                \
  APIFunc API::name { #name,                                                    \
    reinterpret_cast<void *>(&API_##name),                                      \
    reinterpret_cast<void *>(&InvokeReaScriptAPI<&API_##name>),                 \
    reinterpret_cast<void *>(const_cast<char *>(                                \
      #type "\0"                                                                \
      BOOST_PP_SEQ_FOR_EACH_I(DOCARGS, ARG_TYPE, args) "\0"                     \
      BOOST_PP_SEQ_FOR_EACH_I(DOCARGS, ARG_NAME, args) "\0"                     \
      help                                                                      \
    ))                                                                          \
  }

#endif
