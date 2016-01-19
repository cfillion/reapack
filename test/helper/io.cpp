#include "io.hpp"

#include <path.hpp>

using namespace std;

ostream &operator<<(ostream &os, const Path &path)
{
  os << "\"" << path.join() << '"';
  return os;
}

ostream &operator<<(ostream &os, const set<Path> &list)
{
  os << '{';

  for(const Path &path : list)
    os << path << ", ";

  os << '}';

  return os;
}
