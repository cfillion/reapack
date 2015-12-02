#include "io.hpp"

#include <path.hpp>

using namespace std;

ostream &operator<<(ostream &os, const Path &path)
{
  os << "\"" + path.join() + '"';
  return os;
}
