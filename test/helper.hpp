#include <ostream>
#include <set>

class Path;
class Time;

std::ostream &operator<<(std::ostream &, const std::set<Path> &);

// include Catch only after having declared our ostream overloads
#include <catch/catch.hpp>
