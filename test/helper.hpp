#include <ostream>
#include <set>

class Path;
class Time;
class Version;

std::ostream &operator<<(std::ostream &, const Path &);
std::ostream &operator<<(std::ostream &, const std::set<Path> &);
std::ostream &operator<<(std::ostream &, const Time &);
std::ostream &operator<<(std::ostream &, const Version &);

// include Catch only after having declared our ostream overloads
#include <catch.hpp>
