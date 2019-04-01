#include "helper.hpp"

#include <errors.hpp>
#include <path.hpp>
#include <version.hpp>

std::ostream &operator<<(std::ostream &os, const std::set<Path> &list)
{
  os << '{';

  for(const Path &path : list)
    os << path << ", ";

  os << '}';

  return os;
}
