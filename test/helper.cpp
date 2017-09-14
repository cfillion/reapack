#include "helper.hpp"

#include <errors.hpp>
#include <path.hpp>
#include <version.hpp>

using namespace std;

ostream &operator<<(ostream &os, const set<Path> &list)
{
  os << '{';

  for(const Path &path : list)
    os << path << ", ";

  os << '}';

  return os;
}
