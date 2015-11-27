#include "version.hpp"

#include "errors.hpp"

#include <algorithm>
#include <cmath>
#include <regex>

using namespace std;

Version::Version(const std::string &str)
  : m_name(str), m_code(0)
{
  static const regex pattern("(\\d+)");

  auto begin = sregex_iterator(str.begin(), str.end(), pattern);
  auto end = sregex_iterator();

  if(begin == end)
    throw reapack_error("invalid version name");

  // set the major version by default
  // even if there are less than 3 numeric components in the string
  const int size = std::max(3L, distance(begin, end));

  for(sregex_iterator it = begin; it != end; it++) {
    const smatch match = *it;
    const int index = distance(begin, it);

    m_code += stoi(match[1]) * pow(1000, size - index - 1);
  }
}

bool Version::operator<(const Version &o)
{
  return m_code < o.code();
}
