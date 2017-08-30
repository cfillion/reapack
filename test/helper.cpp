#include "helper.hpp"

#include <errors.hpp>
#include <path.hpp>
#include <version.hpp>

using namespace std;

ostream &operator<<(ostream &os, const Path &path)
{
  os << '"' << path.join().toUtf8() << '"';
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

ostream &operator<<(ostream &os, const String &str)
{
  os << '"' << str.toUtf8() << '"';
  return os;
}

ostream &operator<<(ostream &os, const Time &time)
{
  os << time.toString();
  return os;
}

ostream &operator<<(ostream &os, const Version &ver)
{
  os << ver.name().toString();
  return os;
}

CATCH_TRANSLATE_EXCEPTION(reapack_error &e) {
  return e.what().toUtf8();
}
